# B36 — Thử bẻ góc rõ mạnh hơn và bàn giao sớm khi line giữa ổn định

Owner cung cấp video mẫu và yêu cầu sửa theo các ưu điểm quan sát được: đổi
hướng gọn gần đỉnh, đi tiếp nhanh trên nhánh sau, ít quay tìm kéo dài. Video là
screen recording30fps, không có PWM/valid/mode; không xác nhận source hoặc PID
của xe mẫu. Các ngưỡng dưới đây là **trial**, không suy ra trực tiếp từ clip.

B36 dựa trên B35, giữ Base45/Max85, Kp12/Ki0/Kd0,12 và B34 local centroid.

## Hai thay đổi

### 1. Tăng P bổ sung khi còn thấy góc rõ

Giữ eligibility cũ: TRACK, valid directional shape confirmed6ms, error cùng
phía>=1pitch và planner chưa cho thấy đang cải thiện. Tăng ceiling extra gain
0,6→1,0, vẫn dùng `desired = ceiling * clamp(abs(e)-1,0,1)` và ramp0,04/ms.
Không đổi P/D nền hay thêm correction floor, pulse brake hoặc timed turn.

Ở e=1,5pitch, Kd0 để tách riêng P, correction steady23,4→27%; bước tăng gain
mỗi frame2ms vẫn0,08, tương ứng tối đa1,44 điểm correction ở error này. Cue chưa
confirm vẫn P nền. Soft curve không shape, centered0x3C, isolated outer eye và
ALIGN không dùng hỗ trợ này. Cue mất hoặc đang cải thiện nhả extra theo luật cũ.
Steering limit, Max clipping và antiwindup giữ nguyên, common không tăng trên
requested speed. Có thể bẻ sớm/gọn hơn nhưng cue giả có thể làm cắt góc/văng hơn.

### 2. Bàn giao nhanh khi line hẹp ở giữa đủ6ms

Trong TURN/ALIGN hoặc post-turn TRACK, thêm counter `fast_center_ms` cho valid
line chỉ ở bit0x08/0x10/0x18, abs(error)<=0,6pitch, không pending góc khác.
Đủ6ms liên tục thì `fast_exit=1`:

- ALIGN được về TRACK mà không cần chờ legacy12ms mặc định; legacy align_ms
  vẫn là fallback và có thể hoàn tất trước nếu operator đặt thấp hơn6ms.
- Target trở về Base, bỏ cap40/post-turn center60ms trong trường hợp fast exit.
  PWM tăng theo ramp cũ, không nhảy trực tiếp40→45. Không bật straight boost.
- Giữ D đang chạy ở fast handoff để còn phản ứng với error đang về tâm;
  không xóa D chỉ vì hoàn tất fast exit. Legacy handoff cũ vẫn reset như trước.
- Không reset `recovery_ms`, post-turn direction hoặc side memory. Invalid
  xóa counter/fast flag, reloss vẫn retry với budget cũ; không renew arc.
- Error>1,5pitch hoặc confirmed corner mới xóa fast flag, trả về guard cũ.
  Broad central patch0x3C hay glimpse<6ms không earn fast exit. Cua mềm vừa phải
  sau fast exit có thể giữ Base, không tự khôi phục cap chỉ do đổi mask.

B35 direct pivot sau accepted loss vẫn giữ: không forward arc28/80ms, không
minimum spin, valid trả PD ngay. Không ép pivot khi valid line. Không bỏ Start,
fault latch, ADC/UI/motor watchdog hoặc Stop. Đây không đo đủ90°/yaw; narrow
center có thể là line cũ hay nhánh khác, nên fast exit có nguy cơ nhận nhầm.

## Test và build

- Standalone `scripts/test-corner-video.ps1`: **94 ARM assertions pass**.
  Baseline B35 fail210028 vì chỉ có steady correction23,4. Tests gồm mirrored
  corner/soft curves, fast center dt1–4ms, Base ramp, no budget renewal,
  interrupted/broad center, giữ D, custom align4ms, Max/antiwindup/Stop/fault.
- Default suite `scripts/test-control.ps1`: **105332 ARM assertions +8 parser/
  layout +18 receiver tests pass**. Cập nhật đúng expected corner ceiling và
  fast-handoff behavior; giữ GAP D reset, recovery flicker và watchdog tests.
- Dùng executable riêng cho tests mới để giữ emulator map64KiB/count300M/
  timeout60s như cũ, không nới giới hạn để chứa thêm test code. Emulator không
  đo WCET hoặc chuyển động thật; dòng Coverage generic của runner không tự
  chứng minh standalone test bao phủ toàn integration suite.
- Keil **0Error/0Warning**; Code33024, RO1856, RW440, ZI10520bytes.
- HEX end0x08008914, không chạm log từ0x0800C000.
- SHA256: `36EE29522676E0A0E8B7C2C06C85EECBACEF284977AD202466F5644112F45774`.
- Backup actual source và HEX B35: `backups/B35-before-B36-20261029`.
- Chưa nạp board, đo lap hoặc push. Không coi clip mẫu là test xe của owner.

Hardware/ADC/estimator/robot supervisor/menu/driver/Flash schema giữ nguyên.
ControlState thêm counter/flag nội bộ; không đổi record48/header108/CSV.
Known B28 recovery limitations ERR-052/057/058 chưa được ghép lại trong B36;
không tuyên bố đã sửa mọi trường hợp R:2.

## Thử trên xe

Nạp `build/linefollower-B36.hex`, OLED B36, calibration/white reference lại;
giữ45/85 và12/0/0,12. Thử góc sau thẳng, chuỗi góc, cua mềm/nét đứt rồi toàn map.
So với B35 cùng điều kiện. Nếu quay quá/cắt nhánh, không tăng Base để bù; lấy
CORNER CSV và video xe mình trước khi tăng hỗ trợ tiếp. B35 HEX vẫn giữ nguyên
để quay về. Chưa có bằng chứng B36 nhanh hơn16,3s hoặc tương đương xe mẫu.
