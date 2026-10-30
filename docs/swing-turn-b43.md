# B43 — Khôi phục B41 và dùng asymmetric swing khi mất line

Ngày30/10/2026. Owner yêu cầu bỏ B42 (owner báo18s, giật và cua chưa mượt),
khôi phục B41 từ `backups/B41-before-B42-20261030`, rồi thay riêng recovery
đối xứng bằng forward-biased swing. B42 trước sửa lưu toàn bộ source/HEX/hash
tại `backups/B42-before-B43-20261030`. ZIP review B42 đã gửi vẫn giữ nguyên.

## Nền và profile

- Restore190 archive paths của B41 và kiểm byte trước khi sửa, gồm application,
  BSP, config, Keil project, scripts và test suite. Docs/error history được giữ.
- `app/control.h`, `app/line_estimator.c/.h`, `app/robot.c`, blackbox và motor
  driver trở lại B41; không còn active-eye API/validity/mixer của B42.
- Default đúng **Base55 / Max85, Kp12 / Ki0 / Kd0,9**. Menu/ADC DMA2ms/pins
  không đổi. Đây không phải B36 gốc có defaultBase45/Kd0,12.
- Giữ scoped D shaping/calm fast-exit có nguồn từ B37, cùng các sửa timeout
  B38 đã có trong B41. Owner mới nhắc B37 khoảng15,9s; hồ sơ cũ ghi B36 khoảng
  16,11s. Không có artifact/runtime log mới để xác nhận riêng mốcB37; nền của
  lần triển khai này vẫn là B41 theo yêu cầu, không khôi phục lỗi dừng của B37.

## Thay đổi theo yêu cầu

| Tham số | B41 | B43 |
| --- | --- | --- |
| D low-pass | 8ms | 10ms |
| Scoped corner D cap | 34% | 28% |
| TRACK tight reverse ceiling | 28% | 24% |
| Recenter / rejoin cap | 48% | 52% |
| New-corner visible travel | 24ms | 8ms |
| Post-turn center clear | 100ms | 40ms |

`TurnOutput` dùng hai độ lớn độc lập:

| Tín hiệu trong recovery | Rẽ phải L/R | Rẽ trái L/R | Trung bình command mặc định |
| --- | --- | --- | --- |
| Mask khác0 | +64 / −22 | −22 / +64 | +21% |
| Mask0 | +68 / −30 | −30 / +68 | +19% |

Mỗi độ lớn được clip riêng theo Max. Khi Max thấp hơn cả hai độ lớn, chúng
có thể bằng nhau sau clip; không khẳng định positive mean ở mọi Max nhỏ.
Không có brake dwell/timed advance hoặc thêm geometry timer.

`s->speed` được giữ ở `min(previous,48,Max)` làm duty carry cho handoff, không
tính lại từ mean absolute wheel commands. Nhờ đó approach55 vẫn carry48 dù
mask còn đen, không bị đổi thành36. Approach5 vẫn carry5, không nhảy lên48.
Khi valid trở lại, ALIGN nhận Follow ngay, targetrejoin tối đa52 rồi các điều
kiện settle/fast exit của B41 tiếp tục áp dụng. Motor reversal guard giữ nguyên.

Trong TURN, `speed`/`common_x10` của log cũ nay là scalar carry cho handoff,
**không phải trung bình hai wheel commands**. Để tính trung bình command trong
recovery, dùng `(left+right)/2` trên thang permille; cả hai đều không đo RPM.

## Raw replay trên fixture B39

`python scripts/compare-swing-b43.py` compile B41 archive và B43 hiện tại với
header riêng, replay160 raw ADC samples/calibration, CRC32 **1CCA0436**.
Không nội suy. Hai bản cùng Base55/Max85,Kp12/Ki0/Kd0,9; D/corner/recovery
tuning khác nhau như bảng trên. Estimator error/mask/valid khớp ở mọi sample.

| ms | Mask | Valid | B41 L/R % | B43 L/R % | Mean B43 % |
| --- | --- | --- | --- | --- | --- |
| 32540 | F8 | 1 | 85 / 0 | 85 / 0 | 42,5 |
| 32544 | F8 | 1 | 85 / −26,5 | 85 / −22,7 | 31,1 |
| 32548 | F8 | 1 | 85 / −27,7 | 85 / −23,8 | 30,6 |
| 32552 | B8 | 0 | 36 / −36 | 64 / −22 | 21 |
| 32556 | 00 | 0 | 50 / −50 | 68 / −30 | 19 |

Từ32548→32552, bước đổi lớn nhất của một wheel command giảm490→210 permille
(49→21 điểm PWM%). Cận reverse24 chỉ đạt đầy đủ ở error1,65pitch; sample1,646
nên được−23,8%, không ép report thành−24%. Scalar carry tại hai TURN rows là48%.

Kết quả: `build/swing-b43-comparison/b41.csv`, `b43.csv`, `b41-vs-b43.csv`
và `summary.json`. Fixture ghi sample khoảng4ms, không chứa mọi frame2ms;
replay là cùng input đã ghi, không phải mô hình xe sau khi đổi lệnh. Không suy
ra toàn bộ304ms đã được tiết kiệm, điện áp motor thực, bán kính cua hay lap.
Khoảng246mm/boom165mm chỉ là reference map/cơ khí;8ms là tham số thử theo yêu
cầu, không được tính như odometry từ các số đo đó.

## Verification

- `scripts/test-control.ps1` pass:58010 core +48188 IO/UI/log/driver ARM
  assertions; standalone corner94, highKd4955, recovery533, yaw1840,
  branch60, inherited carve810 và new swing715. Python parser/layout8 +receiver18.
- Restore B41 suites, cập nhật numeric pair/target/D expectations có chủ đích.
  Giữ GAP memory, timeout/fault, actuator bounds/antiwindup, ADC/UI watchdog,
  real estimator→Robot→motor, menu và Flash/serial tests.
- New swing tests:hai phía/dt1–4ms/Max10..100; pair64/−22 và68/−30, carry48,
  low-duty no-boost, immediate valid handoff, boundary8ms/40ms,12 chained
  corners, continuous loss timeout và valid-before-timeout. Baseline B41 fail
  **280019** tại expected asymmetric pair; B43 pass.
- Actual motor driver fake-register tests xác nhận chuỗi85/−24→64/−22→68/−30
  và mirror không phát sinh reversal blank khi dấu hai bánh không đổi.
- Keil **0 Error / 0 Warning**; Code34848, RO1856, RW440, ZI10536.
  HEX kết thúc0x08009038, trước reserved blackbox Flash0x0800C000.
- `build/linefollower-B43.hex` SHA256:
  `D88FFC8DA5BE339DDD3F3B394A60F9347244C4C95DF8D7DA38CFC88E1E630AA6`.

## Thử trên xe

Nạp HEX và kiểm OLED **B43**, Base55/Max85, PID12/0/0,9. Thử với pin/calib/map
tương đương B41/B42; đo góc sau đường thẳng và cụm vuông cuối riêng trước khi
tăng Base. Sau nạp, Arm/thu log mới để tránh gán log Flash cũ cho lần B43:
firmware tag CSV là version của exporter, format cũ không lưu recorder tag riêng.

Chưa flash hoặc đo lap B43 trong phiên này. Log/source xác nhận đổi lệnh PWM;
không đủ chứng minh đây là nguyên nhân cơ học duy nhất hoặc sửa dứt điểm18s.
