# B31 — Ưu tiên rẽ cong với hai bánh tiến

Owner xác nhận B30 ổn định chuỗi góc vuông nhưng chậm hơn mốc B28 16,3 giây;
muốn góc 90° được đi theo cung thay vì dừng bánh rồi xoay. B31 giữ Base 45/Max 85,
PID nền 12/0/0,12 và sửa lỗi recovery budget của B30. Chưa có thời gian chạy B31.

## Phân tích so với B28

- B28 cho initial recovery arc 28 ms rồi mới pivot; B30 rút confirmed arc còn
  12 ms. Điều này chuyển sang đảo chiều sớm hơn, trái mục tiêu rẽ cong liên tục.
  Đây là hành vi source, chưa chứng minh nó chiếm bao nhiêu thời gian trên map.
- Cả B28/B30 vẫn có thể đưa PWM bánh trong rất thấp khi PD correction lớn.
  Cùng input synthetic pureP, line ở mép sau confirmed shape cho 85%/1%.
  Vì vậy không có BRAKE dwell không đồng nghĩa cả hai bánh vẫn tiến đều.
- Không có log B30 mới; không kết luận từ quan sát rằng cần tăng Kp/Kd/Base.
  Tăng Base từng khiến owner chạy chậm hơn do phải tìm lại line.

## Cách phối hợp hai bánh

### Khi còn line valid

Ordinary TRACK/cua mềm/nét đứt giữ cách điều khiển B30. Chỉ góc đã có pending
được xác nhận, error cùng phía >=1 pitch, hoặc đang bắt lại cùng phía từ rolling
recovery, mới dùng mixer giữ PWM tiến cho bánh trong.

```text
inner = min(0.4 * commanded_speed, 0.25 * Max)
steering_limit = min(commanded_speed, (Max - inner) / 2)
correction = clip(PID, -steering_limit, steering_limit)
common = clip(commanded_speed, inner + abs(correction), Max - abs(correction))
L/R = common ± correction
```

Antiwindup dùng steering_limit mới ngay từ trước quyết định tích phân. Không thêm
I khi saturated, không đổi gain nền hoặc ép hướng lái. Khi error về gần tâm/đổi
phía không còn đủ điều kiện, ordinary PD lấy lại quyền ngay.

Ở commanded_speed=45 và Max=85, floor là 18%; correction mạnh có thể cho
85%/18% thay vì 85%/1%. Dùng thêm headroom cho bánh ngoài, không bắt bánh trong
về 0 theo mixer trong phạm vi này. Mean PWM có thể lên 51,5% dù target Base vẫn
45%; đây là hệ quả phối hợp bánh, không phải straight boost hay tốc độ đo được.
Khi về tâm, target vẫn 45, không giữ mean cao đó ở đường thẳng.

Floor không áp dụng cho mọi sample TRACK: mắt mép đơn chưa có confirmed cue vẫn
là ordinary PD; cue giả/line đổi phía có thể thoát rolling. Không khẳng định mọi
góc luôn giữ hai bánh quay thật.

### Khi line invalid sau góc đã xác nhận

- Có forward entry đủ điều kiện và không phải post-turn retry: cho cơ hội rolling
  tối đa **80 ms** trong budget recovery cũ. Ratio steering 0,6: ở entry mean45
  là khoảng **72%/18%**, thay vì 81%/9% của ratio0,8.
- Mean entry có thể cao hơn45 do mixer visible; cùng trace dưới đây, rolling
  dùng khoảng82,4%/20,5%. Clip Max theo ratio1,6 trước xuất PWM.
- Nhận valid ở frame nào thì forward PD ngay frame đó. **80 ms là giới hạn tối
  đa, không phải delay phải chạy hết, không phải đo đủ góc/quãng đường.**
- Không bắt lại sau80ms mới dùng pivot search50/36 fallback. Loss chưa có
  pending/split xác nhận giữ arc28ms/ratio0,8 như cũ. Post-turn retry không được
  cấp lại arc mới; center flicker/retarget không renew deadline1.200ms.

Không thay cách nhận diện line, estimator, ADC/DMA2ms, GAP300ms, Stop/watchdog,
reversal guard, SWD, menu, Flash/CSV layout. Không tăng Base trong lượt này.

## Bằng chứng tự động

Test `tests/test_rolling.c` trên B30 fail **160030**: sau20ms loss đã pivot.
B31 kiểm tra hai phía, dt1–4ms, valid rejoin, floor có phạm vi, Max hạ khi đang
chạy, antiwindup, fallback sau80ms và fault thật vẫn latch.
`test_arc.c` kiểm tra reacquisition ở mọi mốc2ms trong cả80ms; chuỗi góc và
budget mới của B30 vẫn được test, không rollback về bug trước đó.

Chạy `python scripts/compare-rolling-b31.py`. Cùng input pureP/error và valid/mask,
source/header/tuning B28/B30 lấy từ backup, B31 dùng source hiện tại:

| Lệnh PWM trái/phải | B28 | B30 | B31 |
| --- | ---: | ---: | ---: |
| Line valid ở mép sau confirmed cue | 85/1 | 85/1 | 85/18 |
| Loss +20ms | 77,4/8,5 | 50/-50 | 82,4/20,5 |
| Loss +34ms | 50/-50 | 50/-50 | 82,4/20,5 |
| Frame đầu bắt lại valid | 58/22 | 58/22 | 63/27 |

Số liệu là requested PWM, không có plant/motor dynamics, không dự báo lap time,
không chứng minh quỹ đạo parabol hoặc vị trí xe. Soft-curve command trước cue
khớp baseline trong trace. CSV so sánh tại `build/rolling-b31-comparison`.
`compare-corner-b30.py` dùng B30 archive khi có, tránh gắn nhãn B31 thành B30.

- **105.133 ARM assertions +8 parser/layout +18 receiver tests pass.**
- Keil **0 Error / 0 Warning**; Code33.476, RO1.856, RW440, ZI10.528bytes.
- HEX kết thúc0x08008AD8, dưới vùng log0x0800C000.
- SHA256: `193350D472ED05574CE984FAA70ED437FD21ACD2AC7B4FAF5447FC1E50D69740`.
- Backup source đang dùng, gồm các thay đổi B30 chưa commit:
  `backups/B30-before-B31-20261029/B30-source.zip`, kèm HEX B30.
- Chưa nạp board, chưa đo timing/vận tốc/quỹ đạo/lap, chưa push B31.

## Thử trên xe

1. Nạp `build/linefollower-B31.hex`, OLED B31; calibration/white reference lại.
2. Giữ45/85 và12/0/0,12, thử một góc90° sau thẳng, rồi chuỗi góc đã ổn định,
   nét đứt/cua mềm và cuối cùng toàn map. So với cùng điều kiện pin/map của B28.
3. Owner trả lời **chưa đo** PWM tối thiểu quay đều. 18% chỉ là trial; cần quan
   sát bánh trong. Không có encoder nên PWM dương không đảm bảo motor đang quay.
4. Nếu cung bị rộng hoặc bắt nhầm line, gửi LOSS/CORNER CSV và vị trí trên map;
   không tăng Base để bù. Rolling80ms có thể đi xa hơn trước nếu hướng/cue sai.

Đi góc90° theo cung có thể rời đúng vệt đen trong chốc lát và cần bán kính quay
phù hợp thân xe/map. Với một hàng sensor, không encoder/IMU, không thể bảo đảm
vừa giữ nguyên vận tốc vừa ôm sát mọi góc vuông. B31 là thử nghiệm thay đổi cách
điều khiển theo mục tiêu rolling, thành tích phải xác nhận trên xe.
