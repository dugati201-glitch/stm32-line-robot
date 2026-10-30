# B33 — Phục hồi điều khiển B28, giữ sửa lỗi recovery

Owner báo B32 hoàn tất map **17,4 giây**, chậm hơn mốc B28 **16,3 giây**, cua
mềm văng hơn. Đây là kết quả owner cung cấp; chưa có CSV/lap lặp lại để phân bổ
1,1 giây chênh lệch cho từng nhánh. B33 không thêm một thuật toán cua thử nghiệm.

## Vì sao đổi lại

B31/B32 giữ bánh trong tiến bằng floor và tăng common PWM khi cần thêm steering
headroom. Cùng pureP input ở mép sau confirmed cue, B28 xuất85/1, B32 xuất85/18:
mean tăng43→51,5%, chênh PWM giảm84→67 điểm. Đồng thời forward loss arc tăng
28→80ms. Những thay đổi này có thể làm cung rộng và đi xa hơn trước khi bắt lại;
command trace không chứng minh bán kính/vận tốc thật hoặc nguyên nhân duy nhất.

Phản hồi phần cứng mới cho thấy hướng thử này chưa đạt mục tiêu. Không giữ bánh
trong18% chỉ để tránh PWM0; bỏ yêu cầu đó trong controller hiện tại. Chưa đo
minimum running PWM và không có encoder để xác nhận bánh có quay thật.

## Phần khôi phục và phần giữ lại

| Thành phần | B33 |
| --- | --- |
| `Follow()` — P/D, gain scheduling, antiwindup, mixer | Lấy nguyên byte source B28 |
| `TurnOutput()` — initial arc và pivot fallback | Lấy nguyên byte source B28 |
| ExtraP | Cue confirmed6ms, rise0,04/ms, trần0,6, planner gate của B28 |
| Bẻ lái khi mới thấy line | PID nền xử lý mỗi frame2ms, không chờ6ms để bắt đầu PID |
| Common / inner-wheel floor | Không floor18%, không nâng common trên requested Base |
| Initial arc | 28ms/ratio0,8; valid chuyển PD ngay, không timed advance/BRAKE dwell |
| B30 budget cho góc mới | Giữ qualification và direction memory |
| B32 thoát recovery khi valid±1500 | Giữ continuous40ms inner valid và valid-before-line-timeout |
| Fault / watchdog / Start | Giữ latch, deadline khi invalid, explicit operator Start |

Base45/Max85, Kp12/Ki0/Kd0,12, D15ms, planner40ms giữ nguyên. GAP/ADC/estimator,
menu/driver/SWD và Flash/CSV không thay đổi. Đây **không phải rollback toàn repo
về B28**: lỗi recovery đã xác nhận không được mang trở lại.

“PID góc vuông” thường được dùng để mô tả luật lái mạnh hơn khi nhận diện cua,
không xác định một bộ gain dùng chung cho mọi xe. B28 đã có bounded gain
scheduling khi shape có hướng được xác nhận. B33 giữ cách đó; không tăng Kd
tùy ý hoặc khẳng định sẽ giữ đuôi xe ổn định chỉ bằng tên thuật toán.

## Validation

- Test phục hồi mới fail **180028** trên B32 vì gain đã tăng ngay frame đầu.
  B33 kiểm tra đúng xác nhận6ms, mean không nâng trênBase45, không floor18%,
  arc28ms, Max/antiwindup/Stop hai phía, dt1–4ms.
- `python scripts/compare-b28-restore.py` biên dịch riêng B28/B32/B33 với
  source/header/tuning tương ứng, dùng cùng64-frame cue/edge/loss/rejoin.
  **B33 khớp toàn bộ 64 frame B28 cả pureP lẫn defaultPD12/0/0,12**: correction,
  L/R, target, mode và arc flag. Không có mô hình động học/lap prediction.
- Hai hàm `Follow`/`TurnOutput` được kiểm tra byte equality với ZIP B28. Tests
  B30 chain, B32±1500/deadline/flicker, GAP/watchdog/calibration/Start vẫn pass.
- Tests `test_rolling.c` của floor/80ms B31 được ghi rõ là historical và thay
  trong default suite bằng `test_b28_restore.c`. `test_arc.c` quay lại kiểm tra
  mọi mốc2ms trong28ms; không bỏ test fault thật. Không tăng map/budget emulator
  để chứa đồng thời hai policy mâu thuẫn trong một default test executable.
- **104.188 ARM assertions +8 parser/layout +18 receiver tests pass.**
- Keil **0 Error / 0 Warning**; Code33.292, RO1.856, RW440, ZI10.528bytes.
- HEX kết thúc0x08008A24, không chạm log từ0x0800C000.
- SHA256: `0579F4C6FA6496832D71A8E953A3DE08822C7BA185830F23EEB4A0A520B0D3B0`.
- Backup actual uncommitted B32 và HEX:
  `backups/B32-before-B33-20261029`. Chưa nạp board, chưa push hoặc đo lap B33.

## So sánh trên xe

Nạp `build/linefollower-B33.hex`, OLED B33; calibration/white reference lại.
Giữ45/85 và12/0/0,12, kiểm tra cua mềm, chuỗi góc cuối và nét đứt, rồi đo toàn
map với cùng điều kiện pin/map. Khớp command trên một trace không đảm bảo
thành tích16,3s; guards recovery còn khác B28 theo chủ đích.

Chưa tăng Base, chưa thêm floor/arc dài/gain mới. Bánh trong có thể giảm rất thấp
hoặc bằng0 khi PD cần bẻ mạnh; mất line kéo dài có pivot fallback. Mục tiêu của
lượt này là lấy lại cách bám đã tốt và giữ các sửa lỗi, sau đó mới tối ưu từng
thay đổi có số liệu. Nếu cua vẫn văng, LOSS/CORNER CSV của B33 sẽ giúp phân biệt
shape giả, saturated PD, mất line và recovery thay vì suy đoán từ toàn lap.
