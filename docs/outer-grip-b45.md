# B45 — Outer-eye grip trên nền B43

Ngày30/10/2026. Owner báo **B43 đạt15,5s**, mượt; **B44 mất16,3s**, chậm hơn ở
cua. Theo yêu cầu, backup B44 rồi khôi phục B43 trước khi áp dụng B45.
Chưa có lượt chạy thực tế của B45.

## Nền được khôi phục

- Backup working tree B44, HEX, test/build records và hash tại
  `backups/B44-before-B45-20261030/`.
- Khôi phục và kiểm byte195 đường dẫn source/config/project/scripts/tests từ
  `backups/B43-before-B44-20261030/B43-source.zip` trước khi sửa.
- Giữ docs và error history của B44 làm lịch sử; các test B44 có tên riêng không
  nằm trong default pipeline B45. ZIP review B43/B44 đã gửi vẫn giữ nguyên.
- Firmware B45 có defaults **Base55 / Max85, Kp12 / Ki0 / Kd0,9**; Straight55
  trở lại trường legacy, không bật Cruise. Menu vẫn5 dòng theo B43.
- `TurnOutput` trở lại B43: nonwhite64/−22, white68/−30, mirror theo hướng;
  không có pha58/−40 sau20ms của B44.
- ALIGN không được cấp tight/reverse reserve, branch bias hoặc scheduled corner
  gain. Position-based Kp boost mới bên dưới vẫn áp dụng cho P trong Follow.

## Permission từ mắt ngoài đang nhìn thấy line

TRACK giữ đường cấp quyền từ broad shape đã confirmed của B43. B45 thêm một
đường không cần pending/broad latch khi đồng thời:

- Raw error không giảm về tâm (`outward`).
- `Abs(e) >= 1,65 pitch` và `e * derivative >= 0`.
- Error dương có bit `0xC0`, hoặc error âm có bit `0x03` trong mask hiện tại.

Permission mở giới hạn correction theo **dấu error hiện tại**, không lấy hướng
pending cũ để cấp quyền cho mắt đối diện. Reverse limit tiếp tục có duty weight
35→45%, pitch weight1,15→1,65 và ceiling24%/Max như B43. Permission là một giới
hạn cho mixer; không ép bánh lùi khi gain/correction chưa đủ lớn.

Khi current outer-eye grant không có broad shape (`shape_ms == 0`), common
request giảm theo `1 - 0,20 * tight`; có broad shape vẫn dùng0,35. Common cuối
còn bị giới hạn bởi `Max - Abs(correction)`, nên giảm hệ số không bảo đảm mean
command luôn cao hơn ở mọi sample. Antiwindup kiểm cả giới hạn correction lẫn
hai bánh sau common reduction.

## Nhả grip khi error về tâm

`CONTROL_TIGHT_RETURN_STEP_PITCH` và inward-rate allowance đều bằng0.
Một raw step về tâm sẽ nhả tight/reverse/branch bias ngay trong frame đó, dù
filtered D vẫn còn hướng ra ngoài. Ngược lại, filtered D hướng vào trong cũng
không được cấp tight khi raw step vừa tăng trở lại. Scheduled corner gain được
clear ở frame returning để không giữ thêm lực P của góc cũ.

D filter10ms và applied corner-D cap28% theo scope B43 vẫn giữ. Nhả reverse
không xóa D history và không ép cả hai bánh dừng; live P/D tiếp tục trộn bánh.
Vì D được lọc, giới hạn và motor có quán tính, đây không phải cam kết bánh trong
tăng RPM ngay hoặc thân xe hết văng trong một frame.

## Kp tăng mượt theo vị trí

`Kp_effective = Kp * (1 + 0,35 * clamp(abs(e)/3,5, 0, 1))`.
Nếu scheduled corner gain còn hợp lệ, nhân tiếp `1 + corner_gain` như B43.
Ví dụ Kp12, error0,5 có P6,3; error2,5 có gain vị trí15 trước corner gain.
Ki vẫn mặc định0. Đây là thay đổi với cả cua mềm và TRACK/ALIGN Follow, không
phải hai bộ gain bật/tắt theo một timer góc90°.

## Thoát góc và phân biệt góc mới

| Tham số | B43 | B45 |
| --- | --- | --- |
| Carry ceiling sau TURN | 48% | 55% |
| Recenter / post-turn rejoin cap | 52% | 55% |
| Default Reacquire | 12ms | 6ms |
| Continuous inner-line evidence | 40ms | 16ms |

Carry vẫn là `min(previous,55,Max)`, không boost một approach5% lên55%.
Ở Base50/55, target thoát góc không bị cap dưới Base. Với Base cao hơn55, cap55
vẫn áp dụng; không diễn giải setting này là bỏ mọi giới hạn ở mọi Base.

Giữ policy B44 mà owner yêu cầu: sau completed handoff, cần8ms narrow valid
riêng (tối đa3 mắt, không chạm hai rìa0x81), rồi4ms directional shape và
`e * shape_dir >= 1` mới clear post-turn/budget cho góc mới cùng hoặc ngược chiều.
Broad không được tính vào8ms; đủ8ms được giữ trong lúc xác nhận shape; invalid
hoặc đang ALIGN xóa qualification. Deadline1200ms vẫn độc lập. Continuous inner
evidence16ms và center-clear40ms là các đường hoàn tất hợp lệ khác.

## Evidence và giới hạn

Test B45 compile riêng frozen B43 và current B45, dùng cùng parameters. Các
test mới kiểm isolated eye chưa có cue, expiry của cue, trả về tâm0,001 pitch
trong khi D còn outward, D inward/raw outward, ALIGN không reverse, common0,20
so với0,35, gain số học6,3, Max/low duty/antiwindup/carry và same/opposite corner.
Xem `scripts/test-outer-grip-b45.ps1` và `tests/test_outer_grip_b45.c`.

Regression dùng profile cũ tường minh khi cần; kỳ vọng không-cue/expired-cue
được cập nhật ở nơi B45 có current outer permission. Unit test có tên phiên bản
cũ trong pipeline hiện tại kiểm policy B45, không chứng nhận lại binary lịch sử.
Giữ hardware, ADC DMA2ms, nút EXTI/NVIC, GAP, reversal guard, SWD và Flash layout.

B44 có command giảm common trong ALIGN và mean search nhỏ hơn B43; đây là
cơ chế phần mềm có thể góp phần làm chậm. Chỉ số16,3s/15,5s là owner đo; chưa
có dữ liệu tách riêng đóng góp của từng thay đổi. EYB khác MCU/số mắt/driver;
B45 tham khảo nguyên tắc, không suy ra cùng PWM sẽ có cùng tốc độ12s.

Kết quả test/build, hash và file HEX được ghi trong `docs/validation.md`.
Nạp `build/linefollower-B45.hex`, kiểm OLED tagB45, Base55/Max85 và PID12/0/0,9.
So sánh lap cùng pin/calib/map với mốc B43; Arm và thu CORNER log mới sau nạp.
