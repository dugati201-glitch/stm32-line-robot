# B41 — Tăng quyền lái khi còn thấy vạch góc

Ngày 30/10/2026. Owner cho phép thay các giới hạn B25–B40 theo yêu cầu B41.
Nền triển khai là B40 đã lưu, không phải B36 vừa restore. B36 với profile
55/85,12/0/0,9 được giữ trong `backups/B36-owner-profile-before-B41-20261030`.
B40 source/HEX/hash được giữ trong `backups/B40-before-B41-20261030`.

## Thay đổi so với B40

| Tham số | B40 | B41 |
| --- | --- | --- |
| Tight error start/full, pitch | 1,5 / 2,5 | 1,15 / 1,65 |
| Inner reverse ceiling | 15% | 28%, vẫn chịu Max/2 |
| Directional correction reserve | 0,5 × reverse limit | 1 × reverse limit |
| Common request khi tight | speed | speed × (1 − 0,35 × tight) |
| Shape confirmation | 6 ms | 4 ms |
| Branch bias | Từ confirmed shape | Ramp từ 2 ms, full ở 4 ms |
| Raw derivative filter | 15 ms | 8 ms |
| Corner D cap/rise | 24% / 1600 pp/s | 34% / 2400 pp/s |
| Recenter/rejoin ceiling | 40% | 48% |
| Startup acceleration | 80 pp/s | 150 pp/s |
| Exit acceleration/ceiling | 160 pp/s / 42% | 220 pp/s / 50% |

Giữ default **Base55 / Max85, Kp12 / Ki0 / Kd0,9**. Fast-exit calm ramp240
pp/s của B37 vẫn có điều kiện. Shape4ms áp dụng directional broad cue;
split-invalid loss confirmation6ms vẫn giữ. Outer/current broad evidence được
phép cấp tight khi drive direction đã confirmed và khớp pending direction.
Returning line, TTL, side mismatch và ALIGN vẫn nhả quyền reverse đó.

Antiwindup kiểm tra cả wheel clipping sau giảm common: correction nằm trong
bound mở rộng chưa có nghĩa bánh trong còn thực hiện được yêu cầu. Không tích
phân thêm vào bão hòa actuator. Giữ motor reversal guard theo từng bánh,
timeout/new-corner fixes B38, GAP và Flash record/layout.

Estimator không đổi: `0x98` có hai mắt trống liên tiếp (bits5,6), không phải
một mắt; không tự biến mask này thành valid hoặc nối nhiều nhánh thành một.
B40 short nonwhite PROBE derivative history được giữ. Không áp encoder hoặc
đặt timer di chuyển theo waypoint từ map.

## Kiểm tra

Toàn bộ `scripts/test-control.ps1` pass:

- Control58010 + IO/UI48182 = **106192 ARM assertions**.
- Standalone corner94, highKd4955, B38 recovery533, yaw1840, branch60,
  B41 carve810; Python parser/layout8 và receiver18.
- Suite integration chia hai entry trong cùng AXF, mỗi entry dùng nguyên
  emulator memory64KiB/20KiB, giới hạn300 triệu instructions/60s. Không tăng
  budget để che noncompletion. Bản đầu chạy chung vượt instruction budget;
  tách phần IO/UI cho phép từng phần kết thúc và kiểm tra đủ các assertions.
- Regression B40-on-both-sides fail260035: bánh trong chưa âm sau entry.
- B41 kiểm tra hai phía/dt1–4ms, confirmation4ms, early bias2ms, zero gains,
  low Base, clear return, Max, actuator antiwindup, loss/rejoin, Stop và GAP.
- Actual motor driver fake-register test thêm85/−28 và mirror: bánh ngoài
  vẫn có PWM khi bánh trong chờ reversal guard.

Chuỗi dưới đây chạy cùng input cho controller B40 và B41 thực sự được compile
bằng ARMCC. Samples32542/46/50 được nội suy để mô phỏng frame2ms; không phải
dữ liệu đo. Test riêng dùng chỉ ba samples ghi thật với dt4ms cũng pass.

| ms | Error pitch | B40 L/R % | B41 L/R % | B41 trung bình command % |
| --- | --- | --- | --- | --- |
| 32540 | 1,506 | 85 / 0 | 85 / 0 | 42,5 |
| 32542 | 1,565 | 85 / 0 | 85 / −23,2 | 30,9 |
| 32544 | 1,624 | 85 / −1,8 | 85 / −26,5 | 29,2 |
| 32548 | 1,646 | 85 / −2,1 | 85 / −27,7 | 28,6 |

Đây là replay command với cùng input, không dự báo input xe sau thay đổi output,
không chứng minh wheel speed, current, slip, bán kính cua hoặc lap11,2s.
Common tính trước clip khác với (L+R)/2 khi wheel command bị clip; log
`common_x10` cũ lưu state speed, không phải phép đo common thực tế tại bánh.

`scripts/build.ps1`: Keil0 Error/0 Warning, Code34820, RO1856, RW440, ZI10536.
HEX kết thúc0x08009018, không chạm log Flash0x0800C000.
`build/linefollower-B41.hex` SHA256:
`CEC057F672A5EFB653EF44842A69CB4ACCA4620ECE617162EB2CABA0BFABEFCC`.

## Thử trên xe

Chưa flash hoặc đo lap B41. Nạp HEX và kiểm tra OLED B41, PID12/0/0,9,
Base55/Max85. Giữ pin/calibration/map tương đương và đo riêng góc sau đường
thẳng, răng cưa/cung mềm, cụm bậc thang và nét đứt. Nếu còn overshoot hoặc
giật, thu CORNER CSV/video mới; không tăng đồng thời Base/Kd trước khi đối chiếu.

B41 cố ý cho phép giảm mạnh phần tiến và lùi bánh trong; không cam kết bánh
ngoài luôn tiến với tốc độ cơ học cố định. Filter8ms có thể làm D đổi eligibility
reserve sớm hơn trên jitter; returning-rate gate vẫn được kiểm tra trong tests.
