# B39 — Thêm quyền lái trước khi mất line tại góc gấp

Ngày30/10/2026. Owner báo B38 không cải thiện thời gian; Base20–30 rẽ ngay khi
thấy line, nhưng Base45–55 vượt khoảng2cm rồi mới vào pha quay tìm lại. Khoảng
2cm là quan sát của owner, chưa có CSV/video đồng bộ để phân tách sensor cue,
wheel command, quán tính và slip. Mục tiêu lần này là giảm nhu cầu recovery bằng
điều khiển ngay khi line còn valid; không chỉ tăng Base hoặc Kd thêm.

## Map và thông số nào có thể dùng?

Đã xem ảnh map/quỹ đạo,32waypoints, spec/template và đọc simulator source.
Bản tham chiếu được lưu tại [reference](reference/robotrace-map-20261030/README.md).
Map có nét đứt sau xuất phát, góc sau đoạn thẳng dài, ziczac, cung mềm và hai
chicane có đoạn nối ngắn. Tọa độ chicane trong JSON cho chiều sâu246mm và các
đoạn ngang293/337mm; không dùng ước lượng400mm cũ để áp cùng thời gian cho mọi góc.

Owner xác nhận: bánh gần68mm, boom khoảng165mm, track width nhỏ hơn153mm và
**không có encoder**. Bar width83/pitch11,86mm chưa được đo xác nhận. Vì vậy:

- Không suy vận tốc mm/s, vị trí waypoint hoặc yaw từ PWM percent.
- Không áp Encoder275ticks/rev,2-run odometry, Flash NVS hay code ESP32 theo
  prompt template. Giữ STM32 SPL, control frame2ms và phần cứng hiện tại.
- Không dùng quy tắc ép error0/tiến tới đỉnh theo timer: center sensor rời vạch
  không đo được trục bánh đã đến đỉnh. B04 từng thất bại với advance hình học.
- Simulator đặt vận tốc bánh trực tiếp và sinh encoder từ kinematics; hình
  trajectory không chứng minh motor thật giữ vận tốc/quay gọn hoặc không trượt.
- Package ghi10,23m nhưng polyline nối waypoint khoảng11,97m; ảnh cung tròn và
  tọa độ là reference, không đủ để coi tổng chiều dài/14vùng cua là ground truth.

## Giới hạn của mixer B38

`steering_limit=min(speed, Max/2)` và L/R bị clip không âm. Khi Max85 và mức
lái đã bão hòa, mixer có thể chỉ cho **85/0** dù Base45 hay55. Nếu đã chạm bound,
tăng Kp/Kd không làm chênh lệch command lớn hơn. Với GA25 đang có đà, bound này
có thể góp phần khiến xe đi vượt rồi mới vào TURN; chưa chứng minh đây là nguyên
nhân duy nhất của2cm thực tế. PWM không phải phép đo tốc độ hoặc lực phanh.

Ngoài ra extra-P của B38 bị xóa khi broad shape thu hẹp còn mắt ngoài, dù
pending direction của cùng góc vẫn còn. B39 cho phép tiếp tục ramp hỗ trợ ở
trường hợp đủ evidence bên dưới, thay vì đợi sensor chuyển invalid.

## Luật mới: signed correction có điều kiện

Giữ defaults **Base55/Max85, Kp12/Ki0/Kd0,9**, raw D/planner filter và B37
applied-D shaping. Không đổi target Base, ramp startup, estimator hoặc ADC.

Reserve chỉ được phép khi tất cả điều kiện đúng:

1. Đang TRACK, pending corner chưa hết200ms và có `drive_corner_dir` được xác
   nhận từ visible directional shape6ms. Hướng này phải khớp pending direction.
2. Error cùng phía góc và lớn hơn1,5pitch; có mắt gần mép phía đó (C0 hoặc03).
3. Error sample không đang quay về tâm rõ rệt. Eligibility bỏ qua returning
   step tối đa0,05pitch và inward filtered rate tối đa2pitch/s để tránh nhiễu
   nhỏ làm reserve bật/tắt từng frame. D thật vẫn chạy, không ép dấu correction.
4. `min(speed,target)>35%`. Duty weight tăng liên tục từ35 tới45%; error
   weight tăng từ1,5 tới2,5pitch. Base20/30 không dùng reserve.

Gọi tích hai weight là `q`,0..1. Inner reverse bound là
`r=min(15, Max*0,25)*q` điểm PWM. Chỉ phía correction theo góc được tăng trần
thêm `r/2`; counter-steering giữ bound cũ. Mixer vẫn giảm common theo Max
headroom, L/R clip trong`[-r,Max]`. Ở full reserve/Max85, saturated command
có thể là **85/−15** hoặc mirror; vẫn có tổng L+R dương, không là pivot tại chỗ.

Extra-P tiếp tục ramp về max(B38 broad desired, q*1,0), cùng rise0,04/ms. Khi
không còn broad/tight eligibility thì nhả assist theo luật cũ. Một raw returning
step lớn hơn0,05pitch hủy **reserve mới** ngay, không đợi planner40ms hoặc D15ms đổi dấu;
broad-shape policy B38 vẫn riêng. Valid center/soft curve không ép signed turn.
Đây là PD có output headroom, không phải lệnh quay90° cố định hoặc timed pulse.
Nếu gain user đều0, reserve không tự phát sinh differential command.

`drive_corner_dir` tách khỏi hướng tìm line: B25 từng cho strong single-eye
sample đổi pending direction ngay để cứu next bend. Sample ấy không đủ cấp
reserve theo góc mới; cần fresh visible confirmation. TTL hết, Stop hoặc
BeginRecovery đều xóa quyền này. Không phát minh hướng từ maskFF/split nhiễu.

Motor driver giữ per-wheel reversal guard: chỉ bánh trong có thể nhận command0
một frame khi đổi chiều; bánh ngoài tiếp tục tiến. Không thêm BRAKE dwell cho
cả hai bánh. −15% là command thử, không bảo đảm motor thật đã quay ngược hay
đạt yaw mong muốn. Dòng điện, traction và tải motor vẫn cần kiểm tra trên xe.

Mất line vẫn chuyển sang bounded TURN50/36 như B38; valid trở lại hủy TURN và
nhận PD ngay. Giữ B38 valid-before-timeout, evidence40ms, new-corner24ms,
GAP300ms, fault latch, ADC/UI/OLED/motor watchdog và explicit Start/Stop.
CSV record48/header108 và Flash layout không đổi; left/right đã hỗ trợ signed
value cho recovery từ trước. ControlState thêm một hướng confirmed riêng.

## Bằng chứng kiểm tra

- Test mới trên B38 fail **240031**: output không có reserve tại valid outer
  line sau confirmed cue. Đây là bound/design limit, không là đo overshoot.
- **1840 standalone ARM assertions**: Base/dt1–4ms/hai phía, confirmation,
  signed bound/positive mean, raw-return release kể cả D còn outward, không
  reserve từ single eye/old opposite cue, TTL, low Base/Max, zero gains,
  antiwindup/Stop và valid reacquisition. Test development từng bắt được lỗi
  cho strong opposite eye mượn quyền góc cũ; `drive_corner_dir` chặn tình huống đó.
  Synthetic error2,48/2,52pitch kiểm tra reserve không tắt chỉ vì jitter nhỏ;
  raw inward0,1pitch vẫn nhả ngay khi filtered D còn outward. Threshold này là
  trial, không làm deadband cho P/D, không xác nhận hết giật vật lý.
- **106137 integration ARM assertions**,94 corner,4955 high-Kd,541 recovery B38
  và8 parser/layout+18 receiver tests pass. Actual motor driver MMIO test kiểm
  tra85/−15 và mirror: outer vẫn PWM khi inner reversal guard; không là đo motor.
- `scripts/compare-yaw-reserve.py` compile B38 từ archive với header/tuning riêng,
  B39 riêng, cùng profile12/0/0,9 và32traces64frames:

| Input script | B38 | B39 |
| --- | --- | --- |
| Base20, confirmed corner, frame8ms | 40/0 | 40/0 |
| Base30, cùng script | 60/0 | 60/0 |
| Base45 hoặc55, cùng script | 85/0 | 85/−15 |
| Ordinary soft curve/no-cue outer/dash ở20/30/45/55 | Command trace | Khớp toàn bộ trace B38 |
| Line quay về trong scripted corner | Normal PD | Khớp trace trở về B38 |

Các số là requested PWM percent. Không phải vận tốc, bán kính hoặc thời gian
vòng mô phỏng; không xác nhận hết vọt2cm. Low-Base/soft equivalence chỉ áp dụng
những input scripts đã chạy, không chứng minh mọi tình huống map sẽ giống nhau.

- Keil **0Error/0Warning**: Code34300,RO1856,RW440,ZI10536bytes.
- HEX end0x08008E10, trước vùng log0x0800C000.
- SHA256: `E62F5B6726925F2DCB128EC07F264E10ED212866953B7BD49170F270AD3810EC`.
- Backup actual B38 source/HEX: `backups/B38-before-B39-20261030`.
- Chưa flash board, đo lap/WCET/current/slip hoặc push GitHub. Chưa xác nhận15s.

## Thử trên xe

Nạp `build/linefollower-B39.hex` từ Keil project hiện tại, xác nhận OLED B39 và
calibration/white reference lại. So với B38 **cùng setting**; có thể thử Base45/
Max85 trước, sau đó55/85; giữ Kp12/Ki0/Kd0,9 để tách ảnh hưởng luật mixer. Chưa
tăng Base60 hay Kd trong lượt so sánh. Quan sát góc sau đoạn thẳng và chicane,
có còn đi vượt rồi tìm lại, có quay quá tay hoặc cắt nhánh không; sau đó đo
ba lượt toàn map, không chỉ chọn một lượt nhanh nhất.

CORNER CSV nên Arm trước khi tiếp cận chính góc đang cần đo, Save khi Stop
trước mất nguồn, Export bằng script. Cần đúng cửa sổ trước/sau góc; log freeze
ở event đầu tiên không phải cả lap. Video cùng lượt giúp đối chiếu thân xe với
left/right/error/mask/mode. Không suy ra vận tốc mm/s từ những trường PWM này.

Lệnh: `scripts/test-control.ps1`, `scripts/compare-yaw-reserve.py`, `scripts/build.ps1`.
