# B42 — Active-eye mixer cho STM32 8 mắt

> **Đã bỏ theo yêu cầu owner:** owner báo khoảng18s, xe giật và cua chưa mượt.
> Bản hiện tại là [B43 trên nền B41](swing-turn-b43.md). Nội dung dưới đây là
> hồ sơ triển khai B42; ZIP review B42 trước đó vẫn được giữ nguyên.

Ngày30/10/2026. Owner yêu cầu triển khai hướng mới sau review EYB, thay luật
visible-corner B36–B41. B41 trước sửa được lưu source/HEX/hash tại
`backups/B41-before-B42-20261030`. Đây là bản thử; chưa có kết quả lap B42.

## Cấu hình để thử

| Thông số | B42 |
| --- | --- |
| Base / Max trên OLED | 55 / 85 |
| Kp / Ki / Kd mặc định | 12 / 0 / **0,10** |
| Derivative filter | 4ms |
| Applied D ceiling | ±24 điểm PWM% |
| Corner forward ceiling tại mắt ngoài cùng | 35% |
| Common recovery/startup ramp | 200 điểm PWM%/giây |
| Strength hysteresis on/off | 600 / 450 trên thang0..1000 |

Kd0,10 là điểm thử theo luật mới, không phải phép quy đổi chính xác từ EYB
hoặc khôi phục gain0,9 của B36/B41. Ki0 lúc thử. Menu PID/Base/Max, wiring,
OLED, ADC DMA2ms và motor driver giữ nguyên cách dùng.

## Luồng điều khiển

1. Estimator chuẩn hóa8 ADC theo calibration/white reference. Mask vẫn dùng
   ngưỡng500 và giữ nguyên các khoảng trống thật. Một cụm dùng connected
   centroid B34; hai cụm chỉ được dùng khi chênh tổng hai phía ít nhất300
   strength units và ít nhất25% tổng strength. Cụm cân bằng, hơn hai cụm,
   white hoặc maskFF vẫn invalid. `position_confidence`:0 invalid,1 single,
   2 directional split. Confidence2 là observation có hướng, không chứng minh
   một đường hình học liên tục; không suy ra nhánh cần đi ở mọi intersection.
2. `Control_StepSensors` nhận cả strengths, không suy active eye từ centroid.
   Per-eye hysteresis600/450; chọn mắt xa tâm nhất của phía có tổng strength
   hơn bên kia ít nhất150 và25% tổng. Không có temporal confirmation cho mixer.
   Rank±1 là cặp mắt giữa,±2 cặp kế,±3 cặp gần mép,±4 hai mắt ngoài cùng.
3. Forward ceiling giữ Base ở rank0/1; giảm tuyến tính từ rank2 đến35% ở
   rank4. Desired common còn giảm4 điểm PWM% cho mỗi pitch error, floor25%
   hoặc Base nếu Base thấp hơn25. Common giảm ngay, tăng lại theo200pp/s.
   Có startup ceiling riêng, không cho assist bỏ qua ramp khi mới Start.
4. P có gain boost `Kp*(1+0.5*abs(e)/3.5)`. Rank3/4 mở assist theo phía mắt,
   không chờ centroid lên sát mép. Rank3 request bằng nửa reverse limit;
   limit bằng60% launch/Base. Rank4 request bằng85% forward ceiling.
   Kẹp phần P/trợ P trước khi cộng D, rồi giới hạn từng bánh. D có thể giảm
   hoặc nhả reverse; không ép bánh âm lại sau D. Outer ratio giới hạn85%
   theo actual positive partner. Các rank khác có guard95% để signed output
   vẫn có tổng dương khi người dùng nâng Base/Max. Đây là PWM, không phải RPM.
5. Khi thấy lại line trong recovery, mixer mới nhận quyền ngay. ALIGN và
   post-turn chỉ còn bookkeeping cho direction/budget/settling; không giữ
   trần common40/48%, không chặn đổi hướng sang góc mới. Hướng outer-eye hiện
   tại có ưu tiên hơn centroid trong memory tìm lại line. Antiwindup không
   tích thêm vào assist floor, P clipping hoặc wheel clipping.

Ở D0/Base55, F8 có centroid khoảng1,5pitch nhưng active eye+4: lệnh khoảng
**L+35% / R−29,7%**, mirror khi line phía trái. Khi error đang giảm, D âm nhả
reverse; khi mắt về giữa thì bỏ corner ceiling và hồi common. Đây là quay
gắt gần pivot, không cam kết hai bánh luôn tiến hay quỹ đạo parabol.

Max là bound; valid TRACK không lấy Max làm straight target. Tăng Base sau
này có thể tăng đường thẳng/cua mềm trong khi trần ở mắt ngoài vẫn35%. Không
bảo đảm mọi Base cao đều ổn định vì chưa đo tốc độ, dòng motor hoặc traction.

## Những phần vẫn được bảo toàn

- GAP300ms, filtered held direction qua nét đứt, short ambiguous/curve bridge.
- B38 valid-before-timeout,40ms visible evidence và bounded recovery1200ms.
  Lost thật vẫn search; line lost kéo dài vẫn có fault latch/explicit Start.
- Start/calibration interlocks, ADC/UI/motor watchdog và per-wheel reversal guard.
- Không thêm phanh dwell cả hai bánh, timed advance hoặc waypoint/encoder giả.
- Blackbox record48B/header108B, hai slot8KiB ở0x0800C000 và CSV27 cột.
  Trong B42, flags bits5..7 chứa active-eye magnitude0..4 khi observation valid;
  `side` là dấu của cue hiện tại khi có, nếu không thì hướng nhớ. Bits0..4 giữ
  nghĩa cũ. Terminal row là snapshot cuối, không phải sample sensor mới.
  `common_x10` là common request trước wheel clipping, không là tốc độ đo.

Motor guard có thể đưa riêng bánh đổi chiều về0 một control frame. Bánh ngoài
vẫn nhận lệnh tiến. Code không xác nhận motor thật đã quay ngược hoặc dừng cơ học.

## Verification

`scripts/test-control.ps1`: **29813 controller/estimator/GAP assertions** và
**48341 IO/UI/motor/blackbox/driver assertions**, chạy ARMCC C dưới Cortex-M3
emulation. Parser/layout8 và receiver18 Python tests pass. Emulator bounds giữ
64KiBFlash/20KiBRAM,300M instructions/60s mỗi entry.

Suite B42 thay các numeric expectation riêng cho mixer B36–B41 đã bị thay thế;
test cũ/source cũ có trong backup, không gọi các test cũ trên luật output mới.
Giữ các suite Robot/Menu/Blackbox/driver và GAP memory; cập nhật các expected
target/gain/header/flags có chủ đích. Không dùng tổng assertion cũ để tuyên bố
hai thuật toán tương đương. Test B42 bao gồm:

- Toàn256 masks ở hai polarity, thêm polarity từng mắt xen kẽ; confidence,
  calibrated endpoints, weak connected shoulder/disconnected island và flat calib.
- Strength on/off/dominance; immediate mirrored entry/retarget; P-before-D release;
  Max10..100/dt1..4/Kp100/Ki2/Kd1, positive signed mean và zero gains.
- Startup, Base changes, soft S-bends, twelve chained bends with loss/rejoin,
  direction memory priority, no-history/timing/turn timeout/fault latch.
- Real estimator→Robot→motor command, steering inversion, explicit Stop,
  menu reset/interlocks, blackbox defaultKd0,10 và real firmware tagB42.
- Giữ regression valid ngay deadline,40ms inner evidence/glimpse không gia hạn
  timeout; balanced ambiguity chỉ được bridge có giới hạn. Driver test thêm
  đúng pair350/−297 và opposite retarget với per-wheel guard.

`scripts/compare-active-eye-b42.py` compile archived B41/current B42 bằng đúng
header riêng của mỗi bản. Replay160 raw samples/calibration từ fixture
`tests/fixtures/b39-corner-1cca0436.csv`, kiểm CRC32 **1CCA0436**. Không nội suy.
Hai bản dùng default riêng: B41Kd0,9/B42Kd0,1, cùng Base55/Max85/Kp12/Ki0.

| ms | B41 valid, L/R % | B42 valid, active eye, L/R % |
| --- | --- | --- |
| 32538 | 0; 55,5 / 51,9 | 1; +4; 35 / −29,7 |
| 32540 | 1; 85 / 0 | 1; +4; 35 / −29,7 |
| 32544 | 1; 85 / −26,5 | 1; +4; 35 / −29,7 |
| 32548 | 1; 85 / −27,7 | 1; +4; 35 / −29,7 |

Trace/summary tại `build/active-eye-b42-comparison`. First-frame history không
biết đầy đủ; replay dùng giả định đã cruising55%. Đây là cùng raw inputs,
không là mô hình động lực học. Khi fixture cố định chuyển all-white, cả hai
bản vẫn vào bounded recovery; không suy rằng trên xe B42 chắc chắn tránh được
loss hoặc đạt12s. Map dùng làm reference, không chạy navigation theo tọa độ.

Keil build: **0 Error / 0 Warning**, Code35036, RO1856, RW440, ZI10544.
HEX kết thúc0x080090F0, trước log Flash0x0800C000. SHA256 của B42 HEX:
`23880C250FC9185EFFE2390903B95A5D277145BA49185D322190C47547DEDDC4`.

## Nạp và đối chiếu trên xe

Nạp `build/linefollower-B42.hex` từ project Keil hiện tại; kiểm tra OLED B42,
PID12/0/0,10,Base55/Max85. Calib/white reference như trước. Thử riêng góc đầu,
zigzag/cung mềm, cụm vuông liên tục và nét đứt; giữ pin và điều kiện map tương đương.
Thu CORNER log/video mới để biết lệnh đổi sớm có làm thời gian cua giảm thật không.
Đừng dùng Kd0,9 của B36 để đánh giá mặc định B42 ngay lượt đầu.
