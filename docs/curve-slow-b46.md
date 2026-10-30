# B46 — Curve slowing trên nền B45

Ngày 30/10/2026. Owner xác nhận B45 đạt **15,2 giây, ổn định 5/5 lượt**;
Base 50 vào cua gọn hơn Base 55. Đây là kết quả owner đo trên xe; B46 cần
được đo lại. Nền B45 là commit `bec27a6` (`15.2s`).

## Baseline và backup

Backup trước khi sửa: `backups/B45-before-B46-20261030/`, gồm `B45-source.zip`,
HEX B45, kết quả test/build và `manifest.json` có SHA-256.
HEX B45: `48476CB6AEC486B0DA797A389DD9E874D1357452682CDD2E9FABDB65C4694D99`.

Defaults B46: **Base 55 / Max 85, Kp 12 / Ki 0 / Kd 0,90**.
`Straight` là trường legacy, không tạo Cruise. Error dùng đơn vị sensor pitch;
PWM là lệnh điều khiển, xe không có encoder để đo tốc độ thực.

## 1. Target giảm theo độ lệch trong TRACK

Nếu Base lớn hơn 49 và `|e| > 0,55`, target hiện tại giảm theo công thức:

```text
target = clamp(target - (|e| - 0.55) * 4.5, 49, target)
```

Target thấp hơn 49 do Base/Max hoặc giới hạn handoff được giữ ở mức thấp đó.
ALIGN vẫn dùng target handoff của B45, chưa áp dụng curve slowing.
Sàn 49% là sàn của **target**. Mixer B45 vẫn giảm `common` theo `tight`
(outer-only 20%, broad 35%) và giới hạn bánh theo Max, nên không có cam kết
mean PWM của hai bánh luôn từ 49% trở lên trong góc gắt.

| Độ lệch (pitch) | Target khi Base 50 | Base 55 | Base 60 |
| --- | ---: | ---: | ---: |
| 0–0,55 | 50 | 55 | 60 |
| 1,00 | 49 | 52,975 | 57,975 |
| 1,65 | 49 | 50,05 | 55,05 |
| 2,50 | 49 | 49 | 51,225 |
| 3,50 | 49 | 49 | 49 |

Base 55 chạm sàn ở khoảng 1,88 pitch; Base 60 ở khoảng 2,99 pitch.
Do đó tăng Base lên 60 vẫn làm đầu cua nhanh hơn Base 55 theo chính công thức
được yêu cầu, không đảm bảo mọi đoạn cua sẽ ở 49–50%.

Khi cần hạ target ở vùng cua, PWM ramp dùng **600 điểm %/giây**, kể cả dữ liệu
đang được phân loại `smooth`. Ví dụ giảm 55 xuống 49 cần khoảng 10 ms trên
lệnh PWM nếu error đủ lớn ngay từ đầu. Đây không phải thời gian giảm vận tốc
thực tế của xe và không tạo thêm một trạng thái brake.

Khi về vùng tâm, target trở về Base ngay; `stable_ms` và `speed_guard_ms` không
chặn việc này. PWM tăng theo ramp **220 điểm %/giây**, hoặc **260** khi đủ điều
kiện thoát cua dưới 55%. Nhánh fast exit hẹp, D thấp của B45 vẫn dùng 240.

## 2. Outer grip chốt hướng cho frame mất line tiếp theo

Trong TRACK, khi raw step không quay về tâm, filtered D không hướng vào tâm,
`|e| >= 1,65` và mắt ngoài phù hợp dấu error đang thấy line:

- Giữ quyền `tight`/reverse của B45.
- Chốt `corner_pending_dir` theo dấu error, TTL 200 ms và `RememberSide`.
- Frame trắng kế tiếp chuyển ngay sang `CONTROL_TURN`, bỏ qua ForwardHold 6 ms.

Không có blocking delay mới. “Ngay” là ngay lần xử lý dữ liệu mất line kế tiếp
(chu kỳ danh định 2 ms), không phải cam kết phần cứng phản ứng trong 0 ms.
Motor reversal guard vẫn có thể hoãn đổi chiều một lần cập nhật trên từng bánh.

Latch hướng không ép bánh trong lùi ngay: P/D, duty, Max và antiwindup vẫn giới
hạn mixer. Việc chốt pending cũng kích hoạt phạm vi D cap của B45 sớm hơn;
test kiểm tra cả dt 1–4 ms. Raw error quay về tâm vẫn nhả reserve/gain ngay;
hướng đã nhớ còn TTL để xử lý mất line sau cua như các cue có nhớ khác.

## 3. Giữ D khi hoàn tất centered ALIGN

Giữ lần reset history đầu tiên khi chuyển từ TURN sang ALIGN. Bỏ lần reset
`last_error`, D, integral và planner rate tại thời điểm centered hoàn tất ALIGN.
UpdateDerivative tiếp tục chạy qua lần handoff này, giúp giữ damping đang có.
Không thay đổi reacquire 6 ms, inner evidence 16 ms, timeout 1200 ms, cơ chế
valid trước timeout hay quy tắc góc mới sau narrow travel 8 ms.

## 4. Swing commands

| Dữ liệu khi TURN | B45 outer / inner | B46 outer / inner | Mean PWM B46 |
| --- | ---: | ---: | ---: |
| Còn mask khác 0 | +64 / −22 | **+66 / −24** | +21% |
| Trắng, mask = 0 | +68 / −30 | **+70 / −33** | +18,5% |

Mean pha trắng B45 là 19%, B46 là 18,5%; không hoàn toàn bằng nhau. Đây là
trung bình lệnh PWM, không phải tốc độ tiến hoặc mô-men đo được. Hai chiều
quay được mirror, từng bánh vẫn clip theo Max. Carry vẫn `min(previous,55,Max)`;
không nâng một lần tiếp cận PWM thấp lên 55% khi bắt lại line.

## Kiểm chứng và chạy thử

Test B46 có comparator từ source B45 được backup: bảng target và ramp hai
chiều/dt 1–4 ms; Base thấp hơn sàn; về tâm dù guard còn; outer cue rồi trắng;
wrong-side/returning cue; giữ D và integral khi centered ALIGN hoàn tất.
Các test recovery/GAP/menu/watchdog/Flash và giới hạn bánh tiếp tục chạy trong
`scripts/test-control.ps1`. Các test mang tên B38–B45 kiểm tra invariant trên
controller hiện tại, không đại diện binary gốc của từng bản lịch sử.

`scripts/test-control.ps1` pass toàn bộ: **106.222 integration assertions**
(core 44.128, IO 48.212, motion 13.882), **12.857 standalone assertions** và
**26 Python tests**. Trong đó test B46 mới có 1.736 assertions với comparator
B45; các bài test trước vẫn chạy với kỳ vọng hành vi được cập nhật.

`scripts/build.ps1` build Keil **0 Error / 0 Warning**: Code 35.272,
RO-data 1.856, RW-data 440, ZI-data 10.536. HEX kết thúc tại `0x080091DC`,
trước vùng log Flash bắt đầu `0x0800C000`.

File nạp: `build/linefollower-B46.hex`. SHA-256:
`9B5599FBE2E0C7D54405FFADC4CE89554C20882B301837DFAA2F73A7375C240E`.
Kết quả lưu ở `build/b46-test-final.log`, `build/b46-build-result.log` và
`build/keil-build.log`. Manifest B46 nằm cùng thư mục backup B45.

Đợt thử đầu dùng defaults 55/85, 12/0/0,90 để so với B45. Ghi riêng thời gian
vòng và số lượt mất line; capture CORNER trước đúng góc cần so sánh. Chỉ thay
Base lên 60 sau khi đã có kết quả Base 55; đây vẫn là firmware thử nghiệm,
chưa có dữ liệu chứng minh giảm thời gian so với 15,2 giây của B45.
