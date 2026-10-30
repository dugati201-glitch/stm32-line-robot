# B38 — So sánh Car_Line_2026, sửa recovery và vào góc sớm

Ngày: 30/10/2026. Owner báo B37 dừng lặp lại ở góc thứ ba của chuỗi góc.
Ảnh OLED có B37, F:2/R:2: `FAULT_LINE / LINE_STOP_TURN_TIMEOUT`.
Error trên LIVE tiếp tục phản ánh sensor sau Stop, không phải snapshot của frame
gây lỗi. Chưa có CSV của lần này để xác nhận mask/valid ngay lúc fault.

## Source tham chiếu khác gì?

Đã đọc project `C:/Users/ADMIN/Downloads/Car_Line_2026/User/Car_Line.uvprojx` và
các file thực sự được project trỏ tới: `Lib/User/main.c`, `Lib/Lib/PID.c`,
`sensor_calib.c`, `Engeni.c/.h`, `Lib_Init/adc_init.c`. Không dùng `User/test`
để suy ra thuật toán RUN. Project tham chiếu không bị chỉnh sửa.

| Thành phần | Car_Line_2026 | B37 trước lần sửa này |
| --- | --- | --- |
| Control period | Poll thời gian DWT, mục tiêu 2 ms trong RUN | Frame timer/ADC/DMA 2 ms, freshness và watchdog |
| Base mặc định | 627/1000, khoảng 62,7% PWM | 55%; Max85 là trần mỗi bánh |
| Error | Position trừ 3500, mỗi khoảng mắt là 1000 | Chia 1000 thành sensor pitch trước PID |
| PID mặc định thực tế | Kp0,060/Ki0/Kd0,500, D theo mỗi tick | Kp12/Ki0/Kd0,9, D theo giây |
| D filter | Alpha0,2 mỗi tick, khoảng 8 ms tại tick2ms | Raw low-pass15ms; applied-D shaping của B37 |
| Vị trí line | Trọng số bình phương signal sau noise floor, gom cả 8 mắt | Centroid cụm mạnh liên tục và weak shoulders nối liền; split/FF không valid |
| Cua thường | Giảm Base theo abs(error), floor khoảng 2/3 Base; tăng P/D khi lệch trên1,2pitch | Base cố định, P bổ sung chỉ ở directional shape; có cap40 trong rejoin |
| Nhận góc | Tổng ba mắt ngoài mới tăng so với lịch sử16ms, confirm1tick; bỏ qua500ms đầu | Broad asymmetric mask có mắt giữa, confirm6ms cho memory |
| Góc đặc biệt | Outer tối đa50%; inner khai báo−65% nhưng bị clamp thành−30%; có brake2ms | Valid line giữ forward PD; confirmed loss pivot±50% trắng/±36% có signal |
| Thoát góc | Đợi center rời line cũ rồi xuất hiện lại; hoặc600ms thì về PID | Center±1pitch hoặc fast narrow center6ms; giữ budget qua post-turn |
| Mất line | Dùng vị trí cuối, vẫn tiếp tục điều khiển; không có line-fault supervisor tương đương | GAP, direction memory, recovery deadline và fault latch |

Comment cạnh default PID của source tham chiếu ghi0,120/0,600, nhưng macro và
phép nhân trong RUN cho **0,060/0,500**. So sánh bằng số trên OLED giữa hai
project sẽ sai đơn vị: riêng D tham chiếu0,5/tick tương đương khoảng
0,1 %PWM·s/pitch tại2ms, trước khác biệt filter; không tương đương Kd0,5 của ta.
Đây là đối chiếu công thức, không phải đề nghị đổi gain hoặc bằng chứng nhanh hơn.

Source tham chiếu có ý tưởng đáng thử: trợ lực theo hình dạng/sai số sớm, và
điều khiển hai bánh liên tục. Tuy nhiên nó **có giảm tốc, brake và đảo bánh**.
Không thể kết luận code này hoặc video mẫu luôn đi hai bánh tiến cùng tốc độ.
Video đã trích ở các mốc1,6–2,0s và8,3–10,1s cho thấy đổi hướng gọn rồi tiếp
tục trên nhánh mới; không đọc được PWM/chiều bánh hoặc controller của xe mẫu.

## Lỗi dừng trong B37

Các sửa ERR-052/057/058 từng có trong B30/B32 đã bị bỏ khi khôi phục nguyên
baseline B28; B34–B37 chưa ghép lại. Đây là regression về recovery, không phải
bằng chứng ADC chậm hoặc cần tăng Kd.

- Nhiều góc đã bắt lại line nhưng chưa đủ100ms center/TTL vẫn chia chung budget
  1.200ms. Các góc khác nhau có thể làm hết budget của góc đầu.
- Valid line ở±1,5pitch có thể ở ALIGN mãi vì gate cũ yêu cầu về±1pitch.
- Timeout kiểm tra trước valid branch, nên có thể bỏ qua frame vừa bắt lại line.

Test mới trên source B37 fail **230023**: valid đến đúng deadline vẫn bị fault.
Ảnh phù hợp reason này; không chứng minh đầy đủ diễn biến xe chỉ từ một ảnh.

## Thay đổi B38

1. **Valid frame được PD xử lý trước line timeout.** Không tự xóa budget từ một
   glimpse; invalid kế tiếp vẫn dừng nếu episode đã hết hạn.
2. **Bắt lại inner line liên tục40ms** (`valid`, abs(e)<=2pitch, có0x3C,
   không có0x81) cho hoàn tất ALIGN/xóa budget cũ và trả target về Base qua ramp.
   Giữ D đang chạy, hướng nhớ/post-turn. Invalid/edge ngắt evidence này.
3. Sau completed handoff, valid travel liên tục24ms và opposite visible shape
   confirmed6ms/e cùng hướng>=1pitch mới tách budget cho góc mới. Same-side
   glimpse, invalid retarget hoặc chưa thoát ALIGN không được mở budget mới.
   Một frame chỉ cộng recovery time một lần; timer saturate tại1.200ms.
4. **P bổ sung bắt đầu ramp từ frame shape đầu**, thay vì chờ6ms mới ramp.
   Direction memory vẫn chờ6ms. Đổi/mất shape xóa gain cũ; error đang cải thiện
   theo planner vẫn nhả assist. Không áp luật này cho ordinary soft curve.

Với pure P/e1,5pitch, correction request đầu shape18→19,44%, sau6ms19,44→22,32%;
steady27% như B37. Đây chỉ là PWM request; Kd cao có thể đã làm correction
saturate nên thay đổi này không bảo đảm mọi frame trên xe sẽ quay mạnh hơn.
Nhận shape từ mask không phải sao chép detector raw-history16ms của source mới.

Giữ **Base55/Max85, Kp12/Ki0/Kd0,9**, D shaping B37 và pivot50/36. Không thêm
brake dwell, timed advance, forward arc, inner-wheel floor, center-white gate
hoặc tăng Kd. Valid line vẫn điều khiển trong frame hiện tại; không ép pivot
theo timer khi còn valid. ADC/estimator/motor driver/menu/Flash không sửa.

40ms/24ms là trial evidence windows, không đo yaw90° hoặc xác nhận đang bám đúng
nhánh; một hàng sensor không xác định duy nhất bản đồ. P vào sớm có thể tăng
phản ứng với cue giả, cần thử trên xe trước khi tăng Base.

## Validation và nạp thử

- Integration: **105331 ARM assertions**, thêm8 parser/layout và18 receiver tests.
- Standalone:94 corner/fast-exit,4955 high-Kd và**541 B38 assertions**.
  Gồm dt1–4ms/hai hướng,12 góc riêng, valid tại deadline, moderate curve,
  invalid ngay trước40ms, glimpse/edge không renew, opposite/same-side/invalid
  retarget, interrupted travel, D preservation, Max/antiwindup/Stop/fault latch.
- Các test numeric entry cũ được cập nhật theo ramp sớm có chủ đích; các case
  soft curve/GAP/watchdog vẫn chạy. Không nới emulator map/time/count bounds.
- Keil: **0Error/0Warning**, Code33552/RO1856/RW440/ZI10528bytes.
- HEX end0x08008B28, trước reserved log Flash0x0800C000.
- SHA256: `7594F90DDB8260D6A9950CD40F24DC62221699CCD7C67D2AC34F7D51ECFFDCD7`.
- Backup actual B37 source/HEX: `backups/B37-before-B38-20261030`.
- Chưa nạp board, đo lap/speed/yaw/WCET hoặc xác nhận hết lỗi thực tế. Không
  suy ra mục tiêu15s từ test. Source tham chiếu chưa build/chạy trong lượt này.

Nạp `build/linefollower-B38.hex` bằng Keil project hiện tại, kiểm tra OLED B38,
calibration/white reference lại. Giữ55/85/12/0/0,9, thử đoạn góc thứ ba trước,
sau đó cua mềm/nét đứt và ba lượt toàn map cùng setting. Không đổi Base/Kd cùng
lúc khi so với B37. Nếu còn F2/R2: ArmCORNER ngay trước đoạn cần đo, chạy qua
đoạn, Stop, Save RAM to Flash trước khi mất nguồn rồi Export CSV bằng script.
CSV cửa sổ đầu tiên không phải log toàn vòng; chọn điểm bắt đầu để tránh trigger
ở góc trước. Export log cần giữ trước khi nạp firmware.

Lệnh kiểm tra: `scripts/test-control.ps1` và `scripts/build.ps1`.
`scripts/compare-high-kd.py` đã pin B37 từ backup khi có, để phép so B36/B37
không âm thầm chạy controller B38 dưới nhãn B37.
