# B32 — Không để recovery timeout chặn line valid

Owner báo B31 dừng lặp lại ở cuối chuỗi góc vuông khi xe còn trên line, chưa
chạy hết đích nên **chưa có thời gian B31**. Ảnh có B31, F:2/R:2 và error+1500.
F:2/R:2 xác nhận line recovery timeout. Error trên trang LIVE có thể cập nhật
sau Stop, không phải frozen trace; ảnh không chứng minh mask/valid tại frame lỗi.

## Lỗi tái hiện trong source

1. ALIGN chỉ kết thúc khi `abs(error)<=1000` đủ thời gian12ms mặc định. Một
   line đơn valid ở mắt trong như mask0x20/error+1500 hoặc0x04/error-1500 vẫn
   được forward PD điều khiển, nhưng không thoát ALIGN. Timer recovery tiếp tục
   tăng tới1.200ms rồi Stop dù đang nhận line valid liên tục.
2. Guard timeout được kiểm tra trước nhánh xử lý valid. Frame mới đã tìm thấy
   line ngay tại deadline cũng có thể bị bỏ qua để fault trước khi PID xử lý.

Regression `tests/test_recovery_exit.c` trên B31 fail **170029** với1300ms
valid/error±1500. Đây là bug đã xác nhận trong code, phù hợp triệu chứng trên
xe; chưa có CSV B31 để khẳng định mọi lần dừng thực tế đều do đúng nhánh đó.

## Sửa B32

- **PD nhận line valid trước quyết định line timeout.** Khi chưa có bằng chứng
  đã bám lại đủ lâu, invalid kế tiếp vẫn chịu budget cũ. Không mở lại RUN sau
  fault, không bỏ ADC/timing/UI/OLED watchdog hay Stop.
- Thêm `recovery_visible_ms`: trong recovery/post-turn, đếm liên tục khi line
  valid, `abs(error)<=2pitch`, có mắt trong0x3C và không chạm hai mắt mép0x81.
  Sau **40ms** dữ liệu như vậy, chấp nhận đã bám lại đường: kết thúc ALIGN và
  xóa recovery budget cũ. Không bắt buộc error về đúng±1pitch.
- Khi xóa budget nhờ evidence40ms, đặt `recovery_arc_speed=0` để không dùng lại
  cơ hội arc của episode cũ. Post-turn direction/guard vẫn giữ; invalid hoặc
  edge/out-of-band ngắt bộ đếm40ms. Không phát minh hướng mới từ dữ liệu invalid.
- Cửa sổ center±1pitch/12ms cũ vẫn cho phép handoff sớm, nhưng glimpse12ms
  rồi mất lại không đủ xóa budget. Nhiều glimpse18ms/invalid2ms vẫn phải fault.
- Handoff mới ở±1,5pitch giữ D/planner đang chạy; chỉ centered handoff cũ mới
  reset D như trước, tránh thêm bước thay đổi correction ở cua.

Giữ Base45/Max85, PID12/0/0,12; không sửa mixer rolling B31, arc80ms/ratio0,6,
unknown28ms/0,8, floor trial18%, cách xử lý góc mới B30, GAP, estimator/ADC,
driver, menu, Flash/CSV schema. Không tăng timeout để che mất line thật.

## Validation

- **105.182 ARM assertions +8 parser/layout +18 receiver tests pass.**
- New tests:±1500 valid liên tục qua deadline, dt1–4ms, valid đến đúngdeadline
  rồi invalid (phải Stop), đủ40ms (được tiếp tục), glimpse18/invalid2 không
  renew, valid mắt mép không đủ evidence và invalid sau đó vẫn fault; fault latch.
- Các tests chuỗi góc, rolling80ms/rejoin/Max/antiwindup, GAP, motor/watchdog,
  calibration/Start và blackbox cũ vẫn pass. Không đổi giới hạn emulator.
- Keil0Error/0Warning; Code33.592, RO1.856, RW440, ZI10.528bytes.
- HEX kết thúc0x08008B4C, trước vùng log0x0800C000.
- HEX SHA256: `558545C76C0B0CE5E16CBE329904BEE8D7EAD987719E798FE60EA00A6B193F3D`.
- Backup actual source chưa commit và HEX B31:
  `backups/B31-before-B32-20261029`. Chưa nạp xe, chưa push, chưa đo lap B32.
- `compare-rolling-b31.py` dùng archive B31 khi có để giữ phép so sánh lịch sử.

Nạp `build/linefollower-B32.hex`, kiểm tra OLED B32, calibration/white reference
lại. Giữ45/85 và12/0/0,12; thử đoạn cuối đang gây Stop trước, sau đó chạy toàn
map và mới đo thời gian. Chưa kết luận B31/B32 nhanh hơn16,3s khi chưa hết map.
Nếu còn F2/R2, Arm LOSS trước Run rồi Save/Export sau Stop để lấy actual
valid/mask/mode theo thời gian. Một hàng sensor không xác nhận đúng nhánh map;
continuous inner-line evidence cũng có thể là nhánh khác, nên vẫn cần thử xe.
