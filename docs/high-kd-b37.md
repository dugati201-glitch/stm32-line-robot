# B37 — Profile55/85 và hạn chế D gây giật khi nối góc

Owner báo B36 đạt16,11giây khi Kp12/Ki0, Base55/Max85, Kd khoảng0,8–1,0; chưa
xác định riêng Kd nào là lượt tốt nhất. Sau đó owner bổ sung góc liên tiếp bị
giật hơn setting cũ khi Kd cao. Mục tiêu15s chưa đạt và không thể xác nhận từ
emulator/video xe mẫu. Chưa có CSV/video của chính lượt16,11s.

## Profile và phạm vi sửa

Mặc định mới: **Kp12, Ki0, Kd0,9, Base55/Max85**, straight compatibility55.
Kd0,9 là lựa chọn thử giữa khoảng owner báo, không khẳng định là gain nhanh nhất.
Không tăng P hoặc scheduled-extra ceiling1,0 của B36, không tăng pivot50/36,
không khôi phục forward arc hoặc floor bánh trong. B34 estimator giữ nguyên.

### Điều tiết riêng D ở Kd cao

Raw derivative vẫn low-pass15ms và planner40ms như trước. Tính thêm applied
`d_output` thay vì luôn cộng trực tiếp `Kd * derivative`:

- Chỉ áp dụng khi Kd>=0,5 và có corner pending, post-turn, ALIGN, **hoặc** line
  hẹp ở tâm08/10/18 với abs(e)<=0,8pitch. Nhờ vậy nhiễu giữa hai mắt không đột
  ngột quay lại full-D khi post-turn guard hết sau stable center.
- Desired D clip trong±min(24 điểm PWM, steering_limit). Build cùng dấu tối đa
  1600 điểm/giây, tương đương3,2 điểm/frame2ms.
- Demand giảm thì nhả ngay. Demand đổi dấu thì xóa D dấu cũ trước, tăng D dấu
  mới có giới hạn. Không kéo dài lực quay cũ do chờ slew qua0.
- P/scheduled P vẫn phản ứng ngay, không slew toàn correction. Ngoài phạm vi
  trên, raw D hoạt động như cũ; low-Kd0,12 cũng giữ luật D cũ.
- Max clipping/antiwindup dùng applied D. BeginRecovery/Stop xóa applied term;
  TURN trực tiếp/driver reversal guard không thay đổi.

Đây là heuristic chống cú D lớn, không phải điều khiển yaw đo bằng gyro. Các
góc có P saturated, retarget hoặc tải cơ khí vẫn có thể gây giật. Threshold0,5
và cap24 là trial; không suy ra đây là thông số của video mẫu.

### Hồi tốc nhanh khi fast exit thực sự yên

Giữ điều kiện B36: line hẹp ở giữa6ms mới earn `fast_exit`; không xóa budget
hay side memory. Khi đang post-turn, đã fast exit, không pending góc, mask chỉ
08/10/18, abs(e)<=0,8 và **raw** abs(Kd*derivative)<=8 điểm, dùng ramp240 điểm/
giây tớiBase. Không dùng applied D nhỏ để che raw rate còn lớn.

Trường hợp khác giữ ramp80/160 và các giới hạn cũ. Startup không tăng, không
straight boost, không tăng tốc trên invalid/GAP. Khi hướng còn biến động mạnh,
không dùng ramp240. Không bảo đảm góc/quãng đường/thời gian vòng chỉ từ PWM.

## Bằng chứng kiểm tra

- B36 fail **220032** trên input post-turn±0,5pitch/Kd0,9 vì correction lớn.
  Đây là input synthetic, chưa chứng minh nó giống lỗi trên xe ở từng frame.
- `scripts/compare-high-kd.py` biên dịch source/header/tuning B36 riêng từ ZIP,
  B37 riêng, cùng profile55/85/12/0/0,9 và64frames mỗi scenario:

| Scenario | B36 | B37 |
| --- | ---: | ---: |
| Error luân phiên±0,5pitch/2ms, peak correction request | 42,5% | 9,2% |
| Calm fast exit43→55, thời điểm commanded duty đạt55 | Chưa đạt trong128ms | 50ms |

Số liệu là requested commands, không có motor/vehicle dynamics và không phải
độ giật/tốc độ thật hoặc dự báo15s. Input jitter250Hz là stress case, không
phải tần số nhiễu đã đo trên xe.

- Standalone high-Kd: **4955 ARM assertions**; Kd0,8/0,9/1,0, dt1–4ms, hai
  hướng, đủ lâu qua hết post-turn, D cap/build/release/reverse, low-Kd và ngoài
  scope, raw-D/broad-patch blocking ramp, Max/antiwindup/direct TURN/Stop.
- Standalone B36 corner/fast handoff: **94 ARM assertions** pass.
- Integration: **105327 ARM assertions +8 parser/layout +18 receiver tests**.
  Tests lịch sử45/85/Kd0,12 được pin rõ bằng `TestLegacyDefaults`, không thay
  product defaults để lách test. Menu và current-default header55/85/Kd0,9 được
  kiểm tra; final Robot CSV fixture vẫn legacyKd120 theo setup có chủ đích.
- `scripts/test-control.ps1` chạy integration rồi hai standalone images để giữ
  map64KiB/count300M/time60s mỗi emulator image; không nới bounds.
- Keil **0Error/0Warning**, Code33304/RO1856/RW440/ZI10528bytes.
- HEX end0x08008A30, dưới log0x0800C000.
- SHA256: `A6E57519DC46C5A3BFF9F52C156DB9EAEF4ED6C8F5A1F884DF542F0C1AB53EDC`.
- Backup actual source/HEX B36: `backups/B36-before-B37-20261029`.
- Chưa nạp board, đo lap/WCET/jerk hoặc push GitHub.

ControlState thêm applied-D, không đổi record48/header108 hay Flash layout.
Blackbox vẫn ghi raw derivative và gain yêu cầu; khi đọc B37 phải tính đến
applied-D policy, không suy correction chỉ bằng Kd*derivative. Known B28
recovery limitations ERR-052/057/058 chưa được ghép fix vào lượt này.

## Chạy so sánh

Nạp `build/linefollower-B37.hex`, OLED B37; calibration/white reference lại.
Mặc định đã55/85/12/0/0,9. Thử góc nối nhau trước, rồi cua mềm/nét đứt/toàn map.
So với B36 **cùng profile55/85/Kd0,9**, không so B37 cao tốc với default45/0,12
cũ rồi quy toàn bộ cải thiện cho thuật toán.

Ghi thời gian3lượt cùng điều kiện và số lần văng/mất line, chọn cả độ ổn định.
Có thể chỉnh Kd0,8 hoặc1,0 trên OLED; cả khoảng này đã test bounds chứ chưa đo
ngoài xe. Nếu giật/văng tăng, dùng lại HEX B36 và gửi CORNER CSV/video lượt đó.
Chưa tăngBase60 hoặc hứa15s. Có cơ sở để thử giảm thời gian hồi tốc; muốn xác
nhận lợi ích trên map vẫn cần xe chạy thực tế.
