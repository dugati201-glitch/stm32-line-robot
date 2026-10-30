# B30 — Tách recovery giữa các góc mới và bẻ lái sớm

Mốc thử: Base 45%, Max 85%, PID nền 12 / 0 / 0,12. TRACK giữ target Base,
không có straight boost. Owner báo B28 đạt 16,3 giây; B29 chậm hơn và đôi khi
dừng ở chuỗi góc vuông. Chưa đo thời gian B30 trên xe.

## Bằng chứng và giới hạn chẩn đoán

Ảnh OLED hiển thị B29, `LINE LOST`, `F:2`, `R:2`, PWM hai bánh bằng 0.
F:2 là line fault; R:2 là `LINE_STOP_TURN_TIMEOUT`, giới hạn hiện tại 1.200 ms.
PWM 0 trên ảnh là kết quả Stop khi fault, không chứng minh xe đã phanh trước cua.
Error hiển thị sau Stop không đủ để dựng lại diễn biến trước timeout.

Test mới đưa nhiều góc đổi hướng, mỗi lần mất line 162 ms, bắt lại tâm 12 ms
và chạy trên line thêm 30 ms. Source B29 dừng tại regression `150032` khi cộng
dồn hết ngân sách dù đã hoàn tất các lần reacquisition. Đây là nguyên nhân
đã tái hiện trong source phù hợp triệu chứng; chưa chứng minh mọi lần R:2 trên
xe đều xuất phát từ nó. Nếu xe thực sự không tìm lại line trong 1.200 ms, Stop
vẫn là hành vi đúng và còn được giữ.

## Thay đổi điều khiển

1. **Góc mới có ngân sách riêng khi đủ bằng chứng.** Sau khi ALIGN đã hoàn tất
   bắt lại tâm theo điều kiện cũ, cần ít nhất 24 ms line valid liên tục ở TRACK,
   rồi một valid directional shape ngược hướng góc vừa hoàn tất được xác nhận
   đủ 6 ms và error cùng phía ít nhất 1 pitch. Khi đó xóa budget/post-turn cũ;
   giữ pending và last-side của góc mới. Không chờ đủ 100 ms ở giữa line.
   Invalid ngắt bộ đếm 24 ms. Cùng phía, split invalid, retarget trong TURN/ALIGN
   và những glimpse ngắn không cấp lại ngân sách.
2. **Hỗ trợ lái ngay khi thấy directional shape.** Extra P tăng từ frame đầu,
   rate 0,04/ms, trần `0,6 * clamp(abs(error)-1, 0, 1)`; vẫn chỉ TRACK, error
   cùng phía shape >=1 pitch và planner chưa cho thấy đang cải thiện. Mỗi frame
   2 ms chỉ tăng tối đa 0,08 lần Kp. Đổi/mất shape xóa gain cũ. Việc ghi nhớ
   hướng và chốt recovery vẫn cần xác nhận 6 ms; một mẫu không ép xe vào TURN.
   Cua mềm không có shape, nhóm giữa 0x3C, mắt mép đơn và RECENTER giữ PD thường.
3. **Bỏ nhả P sớm theo raw trend của B29.** Quay lại tiêu chí filtered planner
   của B28; không đổi Kp/Ki/Kd nền. Owner báo B29 chậm hơn, nhưng chưa có trace
   chứng minh riêng cơ chế nhả sớm là nguyên nhân. Đây là rollback có chủ đích
   để kiểm tra, không phải khẳng định đã đo được thiếu lực cua.
4. **Rút đoạn rẽ tiến sau confirmed loss.** Với pending/split đúng hướng đã
   xác nhận, arc fallback sau 12 ms thay vì 28 ms. Steering ratio 0,8 giữ nguyên;
   loss chưa có cue xác nhận vẫn dùng 28 ms. Đây là giới hạn thời gian rẽ tiến,
   không phải lệnh đi thẳng/chờ trước cua. Không thêm BRAKE dwell. Mọi valid frame
   vẫn chuyển sang forward PD ngay, không chờ hết arc/pivot.

Không thay ADC/DMA 2 ms, estimator/calibration, GAP 300 ms, curve/ambiguity bridge,
driver reversal guard, search 50/36, watchdog, operator Start/Stop hoặc Flash layout.
Không tăng timeout để che lỗi và không dùng ước lượng 16 cm để tiến theo delay.
Duty không phải vận tốc; một hàng sensor và động cơ không encoder không đảm bảo
quỹ đạo parabol hoặc nhận đúng nhánh khi nhiều line ở gần nhau.

## So sánh command

Chạy `python scripts/compare-corner-b30.py`. Script dùng ARMCC và emulator với
cùng input synthetic, Kd=0 để tách riêng phần P; không có mô hình động học.

| Candidate | Correction frame đầu thấy shape | Sau 6 ms | Ổn định | Extra gain sau 6 ms error về tâm |
| --- | ---: | ---: | ---: | ---: |
| B28 | 18,00% | 19,44% | 23,40% | 0,264 |
| B29 | 18,00% | 19,80% | 23,40% | 0,176 |
| B30 | 19,44% | 22,32% | 23,40% | 0,264 |

Cua mềm và target 45% không đổi trong trace. Không suy ra lap time từ bảng này.
`compare-corner-exit.py` tiếp tục tái hiện so sánh lịch sử B29 từ source backup,
không gắn nhãn source B30 thành B29.

## Validation và chạy thử

- B29 fail regression chuỗi góc `150032`; B30 pass cùng chuỗi 10 góc (>1.200 ms
  tổng cộng, từng recovery riêng ngắn hơn deadline).
- 104.000 assertions chạy từ ARMCC-compiled C trên Cortex-M3 emulator;
  8 parser/layout và 18 receiver tests pass. Có test dt 1–4 ms, cue cùng phía,
  ngắt valid travel, first-frame assist, arc 12 ms, Max clipping; bộ test cũ vẫn
  kiểm tra center flicker/invalid retarget không gia hạn search, GAP, watchdog.
- Keil: 0 Error / 0 Warning; Code 33.244, RO 1.856, RW 440, ZI 10.528 bytes.
  HEX kết thúc 0x080089F0, không chạm vùng log từ 0x0800C000.
- HEX SHA256: `9383A9E0EFB7BBBE24CC9814D11D0CCAB56FEA15B07C64E47A5566C5557476A8`.
- Backup: `backups/B29-before-B30-20261029/B29-source.zip` và HEX B29.
- Chưa hardware-verified, chưa nạp board và chưa push B30.

Nạp `build/linefollower-B30.hex` (hoặc build project Keil gốc), kiểm tra OLED B30,
calibration/white reference lại. Giữ 45/85 và PID nền để so với B28/B29: thử nét đứt,
cua mềm, một góc sau thẳng dài, rồi chuỗi góc vuông trước khi đo toàn map.
Thử lặp cùng điều kiện pin/map; ghi thời gian và số lần lỗi. Có thể bẻ sớm hơn
nhưng vẫn cắt nhầm nhánh hoặc văng do quán tính. Nếu còn R:2, Arm trigger LOSS
trước Run; sau fault Save rồi Export CSV để phân biệt search thật với cue sai.
