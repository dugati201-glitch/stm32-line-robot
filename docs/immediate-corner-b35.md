# B35 — Dùng nhánh rẽ trực tiếp cho mọi recovery có hướng

Owner muốn áp dụng phản ứng của góc liên tiếp cho các góc khác, giảm tối đa
đoạn hai bánh tiến thêm hoặc rẽ ngay. B35 phát triển từ B34; chỉ đổi cách vào
recovery, không đổi vị trí line/PID/tốc độ cua mềm.

## Hành vi mới

`BeginRecovery()` luôn đặt `recovery_arc_speed=0`. `TurnOutput()` bỏ hẳn nhánh
rẽ tiến28ms: ngay khi đủ điều kiện chuyển TURN và có hướng, xuất lệnh signed
counter-rotation giống chained retry. Không có timed advance hoặc BRAKE dwell.

| Tín hiệu/trạng thái | Lệnh B35 |
| --- | --- |
| Line valid, kể cả góc rõ | Forward PID ngay mỗi frame, không ép TURN khi còn valid |
| Pending góc đã confirmed6ms rồi mất line | TURN ngay frame invalid đầu tiên |
| Invalid split có hướng | Giữ confirm6ms, sau đó TURN ngay, không thêm arc28ms |
| Loss khác đủ điều kiện có hướng | TURN trực tiếp sau confirmation/memory cũ |
| White dash có recent trusted guide | GAP giữ lệnh theo policy cũ, không tự coi là góc |
| TURN thấy valid ở frame kế tiếp | Hủy quay ngay, sang forward PD/ALIGN; không minimum spin |

TURN duty vẫn50% khi mask0 và36% khi có tín hiệu nhưng invalid, clip theoMax;
lệnh hai bánh đối dấu. Common PWM tiến được yêu cầu bằng0, **không có nghĩa
thân xe hết vận tốc tiến ngay**. Driver vẫn guard đảo chiều riêng từng channel;
bánh đang đổi chiều có thể có một update PWM0, không bỏ guard hay thêm StopBrake.
Duty50 khi TURN là độ lớn lệnh tìm line, không là target chạy thẳng.

Sau pivot, handoff dùng `min(Base,40)` và post-turn cap40 cho tới điều kiện cũ,
giống nhánh chained turn trước đây. Ngoại lệ arc giữ Base45 không còn được kích
hoạt. Không ghép tăng rejoin duty cùng thay đổi này. Giữ default45/85 và12/0/0,12.

Không thể khẳng định mọi góc đều được nhận trước khi mất line: chỉ một hàng
sensor, giữ cue/confirmation/memory hiện tại. Không nâng PID/gain hoặc thay
classification trong lượt này. B34 local centroid vẫn bật; B28 recovery
limitations ERR-052/057/058 vẫn chưa được sửa ở B35.

## Kiểm thử

- B34 fail **200030** trên yêu cầu mới: first invalid sau pending vẫn rẽ tiến.
- B35 first invalid cho±50%, không arc, không BRAKE; invalid có mask cho±36%,
  valid trả forward PD ngay. Test mirrored, dt1–4ms, generic remembered side,
  split confirmation, hạMax20%, retry giữ budget, deadline thật và fault latch.
- Default suite thay historical `test_arc.c` bằng `test_immediate_corner.c`
  và `test_gap_memory.c`. Giữ tests lọc D-kick/giữ GAP pair, Max, Stop, driver,
  calibration/Start, chain memory và watchdog; cập nhật đúng handoff cap40.
- **105.329 ARM assertions +8 parser/layout +18 receiver tests pass.**
- Keil0Error/0Warning; Code32844/RO1856/RW440/ZI10520bytes;
  HEX end0x08008860, không chạm reserved log0x0800C000.
- SHA256: `30F18C5365D2B2748424F7C62E7DD084FC89E6AEB18D51AC12CEAB2A6B62B936`.
- Backup actual B34 source/HEX: `backups/B34-before-B35-20261029`.
- Chưa thử xe, đo lap hoặc push GitHub. Test request không mô phỏng quán tính,
  dòng motor, góc quay hay độ văng; đảo chiều sớm hơn có thể làm xe giật/quét lố.

Nạp `build/linefollower-B35.hex`, OLED B35; calibration/white reference lại.
Giữ45/85 và12/0/0,12, thử một góc sau thẳng trước, rồi chuỗi góc, cua mềm và nét
đứt. So với B34 cùng điều kiện; chưa kết luận nhanh hơn trước khi chạy xe.
