# Đối chiếu source EYB và đề xuất trước khi port

> Trial B42 theo review này đã bị owner loại sau kết quả khoảng18s/giật.
> Bản hiện tại: [B43 trên nền B41](swing-turn-b43.md). Nội dung dưới đây ghi lại
> phân tích trước khi triển khai, không phải cấu hình controller hiện tại.

Ngày30/10/2026. Owner yêu cầu phân tích trước, chưa triển khai port EYB.
Nguồn đọc trực tiếp: `C:/Users/ADMIN/Downloads/EYB/src/main.cpp` và
`include/RunSupport.h`. Pasted text được coi là ý kiến đề xuất, không phải
authorization để chạy prompt port nằm bên trong. B41 đã build/test trước lượt
review này; source EYB và firmware STM32 không bị thay đổi trong review.

## Những điểm source xác nhận

- F411/Arduino,16 sensors qua MUX. **15 mắt dùng cho steering**: PR8 front ở0,
  PR9 rear bị loại; offset còn lại−7..+7. Xe STM32 hiện tại có8 mắt−3,5..+3,5,
  không có mắt đúng0 và không có rear eye. Chưa biết pitch/FOV hai xe bằng nhau.
- `getPosition` nhận found theo tổng/peak, không đòi một cụm. Vì peak>=500
  đã kéo sum>=500, sum>=150 không là điều kiện mạnh độc lập. Tất cả front
  strengths sau noise cut200 vào centroid; không đánh giá hình dạng nhánh.
- `activeEyeOffset`: hysteresis600/450, chọn mắt xa tâm nhất của phía có tổng
  hơn phía kia150. Không có debounce thời gian; vẫn phải chờ scan ADC hoàn tất.
- `eyeForwardLimit`: giảm trần tiến từ offset3, về90/255 ởoffset7. Source
  defaults195/255 straight,90/255 edge; curveSlow9 giảm thêm theo centroid.
- `eyeMotorCommands`: trợ P có mức sàn theo mắt, kẹp P trước D, rồi mới trộn
  và giới hạn bánh. Reverse được giới hạn ngay cả khi chưa bật assist. Mắt
  ngoài thêm ratio guard theo partner thực sự dương, không ép reverse lại sau D.
- Khi lost, giữ pair tối đa3ms chỉ khi đủ center history, pair tiến cân bằng,
  không có wing evidence gần đây. Sau đó search theo lastDir. Found trở lại
  chạy mixer ngay, reset/seed D. Không có ALIGN/post-turn như STM32.
- RUN không refresh OLED/Flash, nhưng đọc MUX16×(5us +2 analogRead) và có
  blanking150us khi motor đổi dấu; vòng chạy không có fixed2ms schedule.
- `loadSettings()` có thể thay toàn bộ source defaults bằng Flash values.
  Các gain đọc trong file chưa chắc là gain trên lượt owner báo12s.

## Chỗ cần sửa trong phân tích gửi kèm

1. Mask98 có hai mắt trống liên tiếp. Không thể giải quyết bằng nối một mắt.
2. EYB không giữ tốc độ tiến cao tại góc ngoài: với D0, nó có thể ra +90/−81,
   tức +35,29%/−31,76%, trung bình command chỉ1,76%. Đó là gần pivot, không
   phải bằng chứng về quỹ đạo parabol hay hai bánh tiến liên tục.
3. B40 ALIGN vẫn chạy PD ngay khi valid, không đợi12ms mới lái. Fast handoff6ms
   có thể bỏ cap40; post_turn200ms không đồng nghĩa giữ cap40 suốt200ms.
   B41 hiện đã nâng caps48 và đổi các hằng số khác, cần so với đúng phiên bản.
4. Recovery ramp EYB200 PWM/s là78,43 điểm PWM%/s, không phải200%/s.
5. Nếu **giả định cùng field of view** và x_EYB=2×error_8, chỉ đổi đơn vị:
   Kp36×2/2,55≈28,24; Kd0,08×2/2,55≈0,06275; curveSlow9×2/2,55≈7,06.
   Kd0,6–0,9 trong bảng không phải quy đổi chính xác. Chưa có geometry/gain
   runtime/motor đủ để coi các kết quả này là tuning phù hợp trên xe thật.
6. Kẹp P trước D giúp D âm không bị dư P quá lớn che mất, nhưng final wheel
   saturation và ratio guards vẫn có thể che một phần thay đổi D. Không có
   bảo đảm mọi D âm đều tạo output thay đổi hoặc hết overshoot trong mọi trạng thái.
7. Không kiểm tra groups giúp giữ steering qua V/corner, đồng thời có thể
   lấy centroid ở khoảng trắng giữa hai nhánh thật. Hysteresis/side dominance
   không tự phân biệt intersection với corner hoặc vạch rộng đối xứng.
8. `EYB_RUN_REVIEW.md` mô tả một phiên bản khác (mất line tiến thẳng/finish16),
   trong khi main.cpp hiện search theo lastDir/finish13. Review dựa vào source,
   không áp instruction hoặc kết luận từ tài liệu cũ sang firmware hiện tại.

## Phương án đề nghị

Ưu tiên một bản thử riêng về **active-eye mixer**, giữ B36/B41 để so sánh.
Giữ ADC DMA2ms, wiring, menu, Flash và motor guard của xe hiện tại.

1. Tách centroid để lái mượt khỏi edge cue để quyết định mức quay. Dùng
   strength/hysteresis và side dominance theo tổng đã chuẩn hóa cho8 mắt;
   không lấy một bit outer đơn độc yếu làm góc90 chắc chắn.
2. Tách Base đi thẳng và corner forward ceiling. Khởi đầu Base55, corner
   ceiling khoảng35–40 là mức thử, Max85 là giới hạn. Khi outer cue rõ,
   giảm trần tiến và trợ bánh trong trong cùng frame; khi vào giữa, nhả trợ
   và hồi common mượt. Không tăng Base lên75 ngay khi đổi kiến trúc.
3. Kẹp phần P/trợ P trước D, rồi bảo toàn D giảm quay đến mixer. Reverse
   bound theo forward partner và đúng TB6612 guard. Ki0 lúc thử. Tune D theo
   đơn vị/pitch/filter mới; không chép nguyên0,9 hoặc0,08. Filter3–4ms chỉ là
   candidate, so trace nhiễu trước khi thay8ms B41 hoặc dùng1ms của EYB.
4. Sau khi mixer có phép so sánh riêng, xem estimator: tách `signal_present`
   khỏi `position_confidence`, cho multi-group có hướng đáng tin vào steering
   có giới hạn. T/giao nhau/cụm đối xứng không chọn nhánh tùy tiện. Không ép
   mọi mask có một mắt>=500 thành vị trí tin cậy ở tốc độ cao.
5. Valid mới nhận lái ngay và cập nhật hướng mới; state chỉ quản lý mất tín
   hiệu thật/timeout. Giữ cách vượt nét đứt của xe vì EYB chỉ hold3ms và không
   đủ thay GAP300ms hiện tại. Không bỏ timeout/Stop để giấu line lost.

Đo riêng thời gian corner entry→stable exit, phần quay tìm, số lần văng, và
toàn lap. Lợi ích kỳ vọng là giảm thời gian vào/thoát góc và recovery, không
chỉ tăng duty đường thẳng. Không kết luận thuật toán duy nhất gây chênh12s/16s
vì hai xe khác sensor layout, motor/driver, cơ khí và settings runtime.

## Verification của lượt review

Đọc source và test fixtures liên quan; đã thử gọi `test/pwm_mix/run.py` và
`test/run_support/run.py`. Cả hai dừng trước compile vì không có g++ trong
environment. Không cài toolchain và không tuyên bố test EYB đã pass. Không
upload, không chạy motor. Bảng lệnh được tính từ hàm đã đọc, không phải đo xe.
