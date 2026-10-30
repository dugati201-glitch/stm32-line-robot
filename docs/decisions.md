# Các quyết định kiến trúc (ADR)

Hiện tại: **B39**, xem [yaw-reserve-b39.md](yaw-reserve-b39.md) và ADR-050.
Các mục B38 trở về trước mô tả lịch sử tại thời điểm triển khai.

## ADR-001: Bare-metal C dùng SPL

Trạng thái: đã thống nhất.
Dùng STM32F103C8T6, Standard Peripheral Library và CMSIS.
Không tự đưa HAL, Arduino hoặc RTOS vào dependency. Tách application logic khỏi BSP code.

## ADR-002: Sửa bằng VS Code, build bằng Keil toolchain đã cài

Trạng thái: đã triển khai.
Dùng `linefollower.uvprojx` làm build definition. VS Code gọi `scripts/build.ps1`.
Cùng project này mở được trong Keil. Startup, device header và SPL source được lưu trong repo.
Build thành công chỉ xác nhận compile/link; kiểm chứng phần cứng là bước riêng.

## ADR-003: STBY nối cố định lên 3V3

Trạng thái: đã thống nhất; chủ dự án xác nhận phần cứng.
Không có GPIO STBY trên STM32. Motor A là bánh trái, Motor B là bánh phải.
Hành vi stop phải thực hiện qua PWM/direction input. Firmware không thể ngắt nguồn động cơ
qua STBY; cần kiểm tra hành vi khi cấp nguồn, reset và startup trên mạch thực.

## ADR-004: Không có wheel encoder

Trạng thái: đã thống nhất; chủ dự án xác nhận phần cứng.
Hai động cơ GA25 có thông số 620 RPM và không có encoder.
Triển khai line-position feedback và duty request có giới hạn.
Không cung cấp wheel speed đo được hoặc triển khai wheel-speed PI loop khi không có feedback tương ứng.

## ADR-005: Control timing độc lập với cập nhật màn hình

Trạng thái: đã triển khai acquisition và controller theo frame TIM3 2 ms; execution timing trên board chưa được đo.
Target control period ban đầu là 2 ms; OLED không được làm trễ control deadline.
Scheduling, execution time, jitter và overrun handling cần được triển khai và đo thực tế.

## ADR-006: Triển khai navigation sau khi xác nhận tracking

Trạng thái: đã thống nhất thiết kế.
Triển khai và kiểm tra sensing, motor output, tracking, speed ramping và gap recovery có giới hạn
trước khi làm junction navigation hoặc maze memory. Không coi mẫu toàn nền là ngõ cụt.

## ADR-007: Ba nút dùng EXTI + NVIC

Trạng thái: đã triển khai driver ngày 24/10/2026; build pass, chưa kiểm chứng trên mạch.
Dùng GPIO edge interrupt qua EXTI và NVIC thay vì polling định kỳ để phát hiện nhấn/nhả.
Mapping: PB0/EXTI0, PB1/EXTI1, PB10/EXTI10; IRQ tương ứng là EXTI0, EXTI1 và EXTI15_10.
ISR chỉ ghi nhận edge/timestamp và clear pending đúng line. Debounce, nhấn ngắn/nhấn giữ
và menu event được xử lý ngoài ISR bằng state machine theo elapsed time.
Đọc lại GPIO để xác nhận debounce được phép; không delay hoặc xử lý OLED trong ISR.
Ưu tiên button interrupt thấp hơn control/acquisition. Cần xác nhận active level và pull resistor,
đồng thời kiểm tra bounce, event trùng, nhấn đồng thời và ảnh hưởng đến control timing.
Chi tiết runtime được quy định trong [runtime.md](runtime.md).

## ADR-008: Lấy mẫu theo TIM3 và công bố DMA frame

Trạng thái: đã triển khai source, build pass; chưa đo timing trên mạch.
TIM3 TRGO kích hoạt ADC1 scan PA0..PA7 mỗi 2 ms, không dùng ADC continuous mode.
DMA circular có 16 sample, chia hai nửa 8 sample; HT/TC IRQ copy frame đã hoàn tất.
Frame snapshot được chuyển sang foreground bằng critical section ngắn.
DMA error hoặc hai flag HT/TC cùng pending làm dừng acquisition để tránh xuất dữ liệu không chắc chắn.
Controller và calibration hiện chạy trong frame callback DMA; foreground chỉ nhận snapshot/UI và gửi yêu cầu.

## ADR-009: Giữ OLED profile tham chiếu đã chạy

Trạng thái: đã triển khai source, chưa thử màn thực của repo mới.
Giữ I2C1 remap PB8/PB9, 400 kHz, 128×64, address 7-bit 0x3C và legacy init của project cũ.
Source cũ tên SH1106 nhưng owner xác nhận SSD1306; không dùng tên function để suy ra controller.
Có thể chọn SSD1306 init tiêu chuẩn qua cấu hình, nhưng không tự thay profile đang tham chiếu.
Mỗi lần gửi một page ở foreground, transaction timeout 6 ms; lỗi bus ngừng refresh đến reset.

## ADR-010: Giữ bố cục và thao tác menu cũ

Trạng thái: triển khai source theo yêu cầu owner; build pass, chưa thử menu mới trên xe.
Giữ thứ tự Run, Sensor Monitor, PID Setup, Calibration, Motor Test, System.
N1/N3 điều hướng hoặc tăng/giảm; N2 chọn/edit; giữ N2 dừng và quay lại.
Không mang theo polling, blocking Auto Calib, gain cũ hoặc PID timing của project tham chiếu.
Motor Test chạy riêng từng bánh ở 20% PWM, timeout 2 giây. RUN đã tích hợp; Auto Calib vẫn chưa triển khai.

## ADR-011: Black-line polarity và RUN theo frame

Trạng thái: source đã triển khai theo phản hồi owner; build/emulation pass, RUN chưa thử trên xe.
Owner xác nhận Motor Test hoạt động, nhưng line đen yếu/trắng mạnh ở bản trước.
Đảo mặc định thành raw HIGH = line strength cao. Giữ shortcut đổi polarity để đối chiếu thực tế.
Controller chạy sau frame hoàn chỉnh trong DMA IRQ, dùng dt=2 ms, D filter 15 ms và acceleration ramp.
Gain mặc định Kp=12, Ki=0, Kd=0.08, Base=20%, Max=30%; không kế thừa gain scaling cũ.
N3 Start/Stop trong Run; Start yêu cầu calibration, line valid và frame mới.
Không auto-start sau boot/fault. SysTick giám sát ADC/UI, output có lease 20 ms.
Có bounded gap/search; ambiguous line dừng vì chưa có junction policy.

## ADR-012: White reference rõ ràng và firmware identity B02

Trạng thái: triển khai source, build và emulation pass; chưa xác nhận B02 trên mạch.
Thay giả định raw polarity toàn hàng bằng white reference riêng từng mắt, lấy sau sweep min/max.
N2 nhấn ngắn sau khi N3 kết thúc calibration: đặt cả 8 mắt trên trắng để xác nhận endpoint.
Start chỉ cho phép khi calibration + reference hợp lệ. Run ghi rõ nguyên nhân chưa Start được.
Giữ mapping cũ: N1/N3 chọn mục, N2 Enter, N3 Start/Stop trong Run.
OLED hiện tag B02; build tạo thêm linefollower-B02.hex và SHA256 để tránh nạp nhầm artifact.
ADR-011 và mô tả raw HIGH trước đó là lịch sử, không còn là chính sách polarity hiện tại.

## ADR-013: B03 phân biệt corner recovery, straight gap và ambiguous probe

Trạng thái: triển khai source, build/emulation pass; chưa xác nhận B03 trên xe.
Owner đã chạy được vòng tròn/cua nhẹ với Base 20% nhưng dừng ở cua 90°.
Giữ PD cho tracking thông thường, thêm TURN theo corner cue và direction memory.
Chỉ thoát TURN/SEARCH khi vùng giữa ổn định; partial edge không reset timeout.
Straight GAP giữ duty không tăng, không reset speed mỗi nét; ambiguous PROBE có thời hạn.
Giữ watchdog và giới hạn recovery. Mặc định Base 30%, Max 45%, không tăng gain mặc định.
Hướng sensor đảo phải đảo cả mask và error. OLED thêm R reason, firmware tag B03.

## ADR-014: B04 giữ menu chính và bổ sung tuning theo geometry

Trạng thái: source triển khai, build/emulation pass; chưa hardware-verified.
Giữ sáu mục chính. PID Setup hiển thị đồng thời năm dòng, System N3 mở Corner Timing.
Base/Max và Advance/Align đều sửa được trên OLED, lưu RAM. Không thêm Save to Flash ngầm.
Owner mô tả sensor offset khoảng 16 cm, đoạn nối góc 40 cm và xác nhận cả rẽ sớm/thoát sớm.
ADVANCE có giới hạn trước pivot cho góc L rõ; ALIGN sau reacquisition để không coi một lần thấy giữa là hoàn tất cua.
Default Advance 350 ms tại 30% là estimate cần hiệu chỉnh thực tế, không phải tính góc/quãng đường chính xác.

## ADR-015: B05 thay thiết kế B04 sau regression trên xe

Trạng thái: source và test logic đã thực hiện; cần thử phần cứng B05.
Owner báo cua vuông xoay vòng một bánh, cua nhẹ văng line. Không quy toàn bộ cho geometry hoặc gain.
Bỏ ADVANCE 350 ms và mọi điều kiện bắt buộc thấy trắng/minimum-turn trước khi nhận lại line.
Valid line giữ PD authority; chỉ pivot khi mất line được xác nhận. Pivot target hai bánh đối chiều cùng duty 20%.
Reacquire valid line chuyển ngay RECENTER bằng forward PD; deadline 1.200 ms không reset bởi flicker.
Default Base 20/Max 30 để so baseline; giữ OLED speed edit và thêm LAST RUN.
Các ADR-013/014 là lịch sử bị thay thế ở phần điều khiển góc. Không coi test mask giả lập là chứng minh góc thực.

## ADR-016: B06 speed planner và short brake có giới hạn

Owner báo Max 40% chạy được, duty cao làm văng ở góc; bộ gain tạm 2/0/5 gây lắc và không qua góc.
Thay gain tạm bằng trial 12/0/0,12, Base 30/Max 60; Ki giữ 0, chưa tuning trên xe.
D per second cập nhật trước planner; risk từ error/rate/nhóm rộng hạ target trước khi mất line.
Ramp tăng 40, giảm 600 điểm/giây; boost cần 300 ms thẳng, cooldown 250 ms sau risk/invalid.
Valid line giữ PD; drive/brake PWM giảm tốc, không cưỡng bức quay/phanh hai bánh theo timer.
Recovery entry PWM cao >40%: BRAKE tối đa 12 ms với duty=0/IN1=IN2=High;
valid line hủy pulse ngay. Deadline bao gồm pulse, watchdog/Start interlock giữ nguyên.
Đồng bộ magnitude PWM khi pivot, RECENTER <=20% common, không dùng speed cũ của đường thẳng.
GAP không full brake mỗi nét; một hàng sensor không phân biệt hoàn toàn gap/góc cụt.
Bổ sung B pulse counter ở LAST RUN. Test logic/register request không là mô hình cơ học.
Xem [high-speed.md](high-speed.md), [error-log.md](error-log.md).

## ADR-017: B07 tách noise handling của planner khỏi PD

Owner thử B06: cua mềm ổn, quay 90° lâu, thẳng chậm. Regression tái hiện static middle-eye error 0,5 pitch
không boost vì corridor B06 quá hẹp; jitter D và nhóm giữa bốn mắt cũng có thể giữ tốc thấp.
B07 giữ gain/PD derivative filter và curve cap, thêm planner-only rate filter 40 ms;
nhận thẳng trong corridor 0,65 pitch/180 ms, cooldown 150 ms và xác nhận risk vừa 8 ms.
Error >1/nhóm rộng ra ngoài vẫn giảm ngay; centered 0x3C không tự bị coi là góc.
Ramp tăng 65, giảm giữ 600; pivot target 24%/ramp 350, không thêm thời gian quay cưỡng bức.
Base/Max giữ 30/60 để so cùng duty; không tăng Max để che boost starvation.
Thêm V/T/reason trong status, LAST RUN và OLED; giữ Start/Stop/watchdog/calibration và brake/GAP policy.
Xem [speed-b07.md](speed-b07.md). Test logic không là xác nhận tốc độ/góc cơ học trên xe.

## ADR-018: B08 tăng duty profile, giữ phanh theo phản hồi owner

Owner xác nhận phanh ổn nhưng Base 30%/cua/90° còn chậm, yêu cầu tăng tốc.
Tăng default Base/Max 40/75%, ramp tăng 80; curve caps 32/28%, pivot 30%/ramp 450,
RECENTER min(Base; 26%). Nâng đồng bộ cap vì tăng Base một mình vẫn bị cap profile cũ.
Giữ gain 12/0/0,12, planner B07, loss/GAP/recovery deadlines, brake threshold/pulse/deceleration và driver motor.
Valid line vẫn PD, không thêm delay hoặc thời gian quay bắt buộc. Không thay watchdog/Start interlock.
Đây là trial duty profile; source equality và regression không chứng minh quãng phanh/góc cơ học ở tốc độ mới.
Xem [speed-b08.md](speed-b08.md).

## ADR-019: B09 Base cruise và bounded guide-memory GAP

Owner báo nét đứt/cua nhẹ khựng, không đọc được OLED khi chạy và chọn bỏ boost, dùng Base cao để mượt.
B09 target Base 45/Max ceiling 75; bỏ stability/risk/cooldown gates của tốc độ, giữ PD gain/filter.
abs(error)<=1,5 dùng Base, 1,5→3 giảm liên tục về min(Base;28%) thay các cap dạng bước cũ.
GAP lưu/giữ exact recent trusted forward L/R; filtered rate chỉ đánh giá guide, không dùng D-kick tức thời để loại GAP.
Guide TTL entry 80 ms, edge/drift invalidate, span loss 300 ms, central split grace 12 ms trong cùng deadline.
Valid line nhận PD ngay; timeout là fault cần explicit Start. Không kéo dài vô hạn theo flicker invalid.
Giữ brake driver/pulse/threshold, recovery deadline, ADC/UI/motor watchdog và Start interlocks.
Tests phải kiểm tra lệnh được giữ, không suy từ PWM dương ra bánh còn quay. Xem [cruise-gap-b09.md](cruise-gap-b09.md).

## ADR-020 ? B10: d?ng h??ng t? split mask c? x?c nh?n

Invalid centroid kh?ng ??ng ngh?a mask kh?ng ch?a th?ng tin h??ng. Ch? d?ng asymmetry m?nh c? outer edge li?n t?c 6 ms cho recovery; kh?ng d?ng centroid invalid cho PD, kh?ng ch?n nh?nh giao l?. Gi? timeout v? valid-line authority. Xem [B10](recovery-b10.md).

## ADR-021 — Cập nhật hướng trong recovery và handoff guide

B11 nhận split cue mới trong episode recovery chỉ sau khi đã thấy line valid; không reset deadline khi retarget. Khi RECENTER hoàn tất, lưu guide từ lệnh forward hiện tại theo điều kiện cũ để không có frame mất history. Không đổi gain/tốc độ hoặc tự tăng Base. Xem [B11](recovery-b11.md).

## ADR-022 — B12: last-side fallback trước line fault

Theo đề nghị owner, lưu phía line rõ gần nhất có TTL600ms, clear sau trusted center100ms. GAP vẫn kết thúc giữ lệnh ở300ms; khi còn hướng mới được chuyển recovery1200ms thay vì fault ngay. Đây là heuristic tìm lại, không nhận dạng map hay auto-restart. Chi tiết [B12](recovery-b12.md).

## ADR-023 — B13: phân biệt tracking ổn định với vừa thoát recovery

Lưu hướng và guard200ms sau RECENTER; center tin cậy100ms clear sớm. Reloss trong guard không dùng GAP300ms, tiếp tục recovery có deadline chung. Không chờ bằng delay hoặc đo góc bằng timer. Đây là heuristic có tradeoff với nét đứt ngay sau cua. Xem [B13](recovery-b13.md).

## ADR-024 — B14: cue từ valid shape và diagnostics sau Stop

Centroid valid không đủ phân biệt hình dạng. Mẫu rộng chạm mép lệch bên xác nhận6ms giảm target nhưng giữ PD; pending200ms chặn GAP khi mất dấu sớm. Không pivot trước khi invalid. Thêm DIAG sau Stop để phân biệt nhận dạng/GAP/recovery/timing thay vì suy đoán từ LINE LOST. Chi tiết và tradeoff tại [B14](diagnosis-b14.md).

## ADR-025 — B15: bonus trên nền Base thay tăng Base toàn map

Owner cho phép adaptive speed trở lại. Giữ Base45/Max75/PID đã chạy ổn; thêm bonus tối đa10 có điều kiện, cap55 và steering headroom15. Cua mềm có thể nhận bonus; shape gấp/recovery chặn bonus300ms. GAP giữ exact60ms rồi giảm riêng phần dư vềBase, giữ tỷ lệ bánh; short reacquisition giữ duty. Hệ quả: thay policy fixed cruise/exact300ms của B09; tham số heuristic cần thử xe. Không sửa watchdog, brake hardware hoặc recovery deadline. Xem [B15](speed-b15.md).

## ADR-026 — Tăng defaults theo từng nấc, B16 chỉ đổi50/80

Theo yêu cầu owner, đổi defaults sang50/80, giữ gain và controller B15 để đối chiếu riêng ảnh hưởng profile. Không triển khai gain scheduling hay tăng trần bonus trong lần này. Trần bonus vẫn55; mức GAP nền nâng50. Kết quả trên xe quyết định nấc tiếp theo. Xem [profile-b16.md](profile-b16.md).

## ADR-027 — B17: phân biệt stable curve và outward drift, tăng tốc recovery

Blend từ2→3 pitch, tight/shape32, outward rate4→16 tạo giảm target theo risk; valid PD giữ authority. Pivot36/ramp600 và RECENTER30. Giữ brake12ms/recovery1200ms và memory guards. Defaults55/85; không gain scheduling trong revision này. Owner báo cải thiện1s,60/90 kém hơn; ưu tiên giảm thời gian recovery/khựng trước tăng Base. Xem speed-b17.md.

## ADR-028 — B18: bounded curve snapshot và command-state durations

Owner cho phép tiếp tục tối ưu. Fragmented moderate curve được giữ L/R8ms, white4ms với snapshot age10ms và guards loại corner/edge/rate. Không mở rộng straight GAP hay dùng centroid invalid. Thoát recovery giữa line dùng ramp160 dưới42, sau đó80. TIME cộng frame2ms, CV subset TRACK, không là wall-clock hay brake distance. Giữ55/85/PID/phanh/pivot36. Chi tiết speed-b18.md.

## ADR-029 — Tách Straight khỏi Base và wheel ceiling Max

B19 thêm Straight RAM-only trên OLED; defaults55/60/85, clamp Base<=Straight<=Max. Bỏ hard-coded cap55/bonus10/headroom15; dùng Straight-Base và mixer giới hạn Max. Straight cần corridor0,8 pitch/80ms, guards góc giữ B18. Cua mềm ngoài corridor vềBase, không kéo thêm tốc thẳng theo cua. Heuristic không là geometry lookahead; test xe quyết định mức tiếp. Xem speed-b19.md.

## ADR-030 — B20: entry brake có điều kiện và giới hạn tăng correction

Owner cho phép phanh chủ động khi vào góc. B20 thêm ENTRY_BRAKE8ms trước loss với confirmed shape6ms, fast center memory150ms và mean duty>=50. Đây là ngoại lệ có kiểm soát cho policy cũ không full brake khi valid. Valid thường hủy ngay; invalid dùng phần pulse còn lại và giữ recovery deadline; latch chỉ mở sau trusted center. Soft steering giới hạn build-up800 trong vùng moderate, giải phóng correction ngay, không ép bánh trong luôn tiến. Post-turn cap35 tới60ms center (TTL200ms), không xác định được danh tính line. Giữ55/60/85/gain12/0/0,12 để đối chiếu. Diagnostics phân biệt ZERO/ENTRY; xem speed-b20.md.

## ADR-031 — Forward PID liên tục thay ENTRY_BRAKE khi line valid (B21)

Owner yêu cầu giảm chuỗi phanh/chạy lại. B21 thay ADR-030 ở phần entry pulse: bỏ phanh hai bánh chỉ vì valid corner cue; giữ PID và giảm common duty có ramp. Moderate track dùng deceleration120 khi target≥Base, edge/corner giữ600. Không tạo quỹ đạo parabol giả định. Recovery brake, corner memory, post-turn bounds và watchdog giữ nguyên. Enum/diagnostic ENTRY chỉ giữ tương thích, không phát ở B21. Chi tiết và tradeoff: [speed-b21.md](speed-b21.md); chưa hardware-verified.

## ADR-032 — Hộp đen B22, Flash hai slot và TX-only PB12

Owner đồng ý CH340 và cần giữ log sau khi tháo nguồn. Chọn ring RAM 160×48 byte, cadence4ms với mode/trigger bypass; capture80 sample trước/80 từ trigger. Chọn LOSS hoặc CORNER trên OLED; freeze đến ARM tiếp theo. Lưu Flash thủ công khi dừng vào hai slot8KiB ở 0x0800C000/0x0800E000, CRC32 và commit cuối; firmware bị giới hạn48KiB bởi scatter và HEX checker. Không ghi Flash trong RUN.

Giữ sáu main items; System N2 vào BLACK BOX, N1/N3 chọn, N2 ARM/Trigger/Save/Export. PB12 Software UART TX9600/8N1, stopped-only; không đổi dây đang chạy xe. Pause ADC khi Flash/Export và yêu cầu frame mới trước Start, giữ sticky capture fault. CSV có transport CRC32, profile/calibration và parser trên PC. RAM mới chưa Save chặn Export bản Flash cũ. Giới hạn: một cửa sổ ngắn, PWM request không là đo tốc độ, UI chờ lúc Export, logging có overhead cần đo. Chi tiết: [blackbox-b22.md](blackbox-b22.md).

## ADR-033 — PROBE có giới hạn theo trusted guide cho ambiguity ngắn (B23)

Owner yêu cầu sửa theo raw trace CRC B4544906. B23 thay policy mọi unknown nonzero mask đều lập tức target15 trong một trường hợp hẹp: center history40ms, guide≤10ms, error≤0,8/rate≤4, hai mắt giữa vẫn đen, không corner/recovery/guard hoặc entry SplitDirection. Giữ trusted wheel ratio trong≤32ms, cap32 và không tăng duty, vẫn decel600 nếu đang nhanh. Sau giới hạn hoặc mất anchor dùng policy cũ; confirmed direction/pending có ưu tiên, invalid centroid không được dùng, counters không reset. Valid line tiếp tục PID ngay. Gains/defaults55/60/85, brake/GAP/memory/watchdogs không đổi. Rủi ro: có thể tiến thêm vào intersection/đen rộng; một trace startup không chứng minh whole-map stability. Xem [speed-b23.md](speed-b23.md).

## ADR-034 — Direct turn B24 theo yêu cầu owner

Owner yêu cầu bỏ phanh rồi mới rẽ. B24 supersedes policy giữ recoveryBRAKE ở các ADR/AGENTS cũ: không phát BRAKE/ENTRY trong controlRUN, giữ explicitStop/faultwatchdogBrake. Pivot tách common decel600 và steering slew1800 để thêm yaw ởframeđầu, target±min(Max,36). Valid confirmedasymmetricshape thêm steering floor0,8 khi cùng hướng/e không cải thiện, bỏ cap32 riêng do shape; PD/risk planner giữ authority khi cue clear/improving. Reversal guard theo từng wheel để outerwheel không bị ngắt khi chỉ inner đảo dấu. Giữ bounds/deadlines/squarememory/whiteGAP và không chọn hướng từFF. CORNER recorder đổi trigger để bỏstartupinvalidkhônghướng. Chi tiết/rủiro/validation tại [speed-b24.md](speed-b24.md), chưahardware-verified.

## ADR-035 — Fixed Base và hướng recovery theo bằng chứng quan sát (B25)

- Context: owner báo B24 văng line ở thẳng/góc liên tiếp, yêu cầu bỏ boost rồi giảm tốc. Không có log B24 mới để kết luận động lực học.
- Decision: TRACK target=Base, bỏ Straight bonus/curve-risk target scheduling/shape floor; giữ PD mỗi2ms và startup ramp. Pending loss không chờ thêm6ms nếu không có cue split ngược; TURN request counter-rotation ngay, không ramp tiến. Giữ hướng đã xác nhận trước sai số giữa yếu; hướng mới mạnh/split confirmed vẫn ưu tiên. Xem [B25](fixed-base-b25.md).
- Exceptions: ALIGN30/post-turn35, bounded ambiguity, GAP300ms, recovery1200ms, Stop/fault/watchdog giữ nguyên. Fixed Base không đảm bảo common PWM hoặc tốc độ cơ học cố định khi mixer đạt giới hạn.
- UI:5 dòng Kp/Ki/Kd/Base/Max; legacy straight_percent đồng bộ Base, không dùng boost; CSV/Flash schema không đổi.
- Evidence: baselineB24 fail90048; B25 97520 C assertions +26 Python tests, Keil0errors/0warnings. Cần chạy xe để đánh giá ổn định/thời gian vòng; không khôi phục ADVANCE/minimum-spin B04.

## ADR-036 — Base45, forward PD phản ứng ngay và SWD cấu hình rõ ràng (B26)

- Context: owner báo45 ổn định hơn55, cần giảm thời gian cua; lỗi Keil xảy ra dù chưa Save và thiếuNRST.
- Decision: giữ fixed Base, default45/85; bỏ slew toànPD vì trễ P, giữ D filter15ms. Rejoin giữ command magnitude có giới hạn40 thay vì reset30, post-turn cap40. Không sửa estimator/driver/Flash logger khi chưa có trace chứng minh lỗi ở đó.
- Hardware interface: dùng helper board ghi SWD-enabled field cùng I2C remap, bỏ SWJ_CFG readback write-only. Giữ các thay đổi DBGMCU/main đã có của owner. Hướng dẫn Normal khi thiếuNRST và UnderReset khi đã nốiNRST; không viết option bytes.
- Consequences: valid line vẫn chia PWM hai bánh ngay, quay ngược chỉ ở confirmed loss. Giảm độ trễ có thể tăng nhạy với noise; rejoin nhanh hơn phải thử square-chain. Không có cam kết Base45 luôn giữ được40 trong mọi góc do mixer/Max.
- Validation:101895ARM+26Python tests, Keil0errors/warnings; baselineB25 fail100031. Chưa chạy xe/nạp board. Chi tiết [B26](corner-flow-b26.md), [Keil](keil-download.md), ERR-044/045.

## ADR-037 — Bounded forward arc, stagger pivot reversal và lọc riêng guide nét đứt (B27)

- Owner chấp nhận rủi ro văng thân xe để rẽ ngay bằng duty đang có; báoBase45/B26 cải thiện.
- Decision: forward arc28ms đầu recovery khi entrymean>=35, correction0,8common, clipMax; valid rejoin ngay ởBase, fallbackpivot khi chưa thấy line. Không renew khi post-turn reloss. Không geometryADVANCE/minspin; vẫn deadline1200ms.
- Driver: stagger hai reversal guard khi đổi hướng pivot để giữ ít nhất một PWM tiến; giữ zero mỗi channel và Stop ngay. GAP dùng correction lọc riêng nominal25ms, giữ guide gốc cho ambiguity và livePID.
- Tradeoff: arc có thể trượt/cắt nhánh; cap40 không áp cho arc rejoin thành công. Post-turn retry vẫnpivot để không tiến mù ở chuỗi góc vuông. PWM không đo vận tốc.
- Evidence: baselines fail120032/50056;102331ARM+26Python tests. Hardware/lap time chưa xác nhận; xem [B27](rolling-corner-b27.md), ERR-046/047/048.

## ADR-038 — Visible-corner extra P tăng dần và search hai mức (B28)

- Context: owner báo B27 Base45 đạt17,30giây, yêu cầu kết hợp nhận góc sớm/gain scheduling/quay tìm nhanh và thử nhiều hướng.
- Chọn extraP mượt khi có confirmed valid directional shape, tăng theo abs(error), giới hạn0,6; không đổiKd, không floor ép correction và không geometryADVANCE. Khi cải thiện theo planner/cue không còn thì nhả; PID nền của cua mềm giữ nguyên.
- So sánh ba candidate ARM command-only cho thấy gain mượt có bước extra1,44 thay5,4 của gain đổi tức thì ở input pureP đã chọn. Không có plant model hay dự báo lap time.
- Search50 trên trắng,36 khi có đen chưa valid, forwardPD ngay khi valid. Giữ45/85, B27 arc/dash/driver, rejoin40 và deadlines/interlocks.
- Consequences: metadataKp là gain nền, cần đọc firmwareB28 để hiểuP effective. Có nguy cơ nhận nhầm cue và quét lố; không khẳng định90deg hoặc nhanh hơn trước khi thử xe.
- Validation:104022ARM+26Python, candidate comparison, Keil0Error/0Warning. Chi tiết [B28](visible-corner-b28.md), ERR-049/050. BackupB27.

## ADR-039 — Fresh inward trend nhả extraP trước rate lọc (B29)

- Context: B28 được owner báo16,3giây ở45, cao hơn55/60 không có lợi trênmap. Giữ45/85 làm mốc.
- Decision: tănggain-rise0,04→0,05/ms, giữtrần0,6; xác nhận6ms errorgiảm rõ đểlatch fadeextra sớm. Plateau khôngrebuildgain, cần6msoutward đểrearm; PID nền/memory/deadlines không bị reset bởi trend.
- Evidence: B28 fail140035. So sánhB28/rise-only/B29 cùng synthetic trace: entry19,44/19,8/19,8; steady23,4 cảba; extra sau6msreturn0,264/0,264/0,176. ChọnB29 theo hành vi lệnh, không dự báo thành tích.
- Tradeoff: rise nhanh/return fade có thể làm phản ứng với cue giả hoặc nhả thiếu lực, cần thửxe. Gain metadata vẫn làgain nền, Flash/CSV khôngđổi. Không tăngBase/Max hoặcđổi driver/SWD trong lượt này.
- Validation:104149ARM+26Python+candidatecomparison, Keil0errors/warnings. Chi tiết [B29](corner-return-b29.md), ERR-051; backupB28.

## ADR-040 — Budget riêng cho góc mới có bằng chứng và visible steering sớm (B30)

- Context: owner báo B29 chậm hơn B28, góc sau thẳng vượt line và chuỗi góc dừng
  F:2/R:2. Regression150032 tái hiện nhiều góc đã bắt lại nhưng cộng dồn budget.
- Decision: chỉ kết thúc post-turn episode cũ khi completed ALIGN, >=24ms valid
  TRACK liên tục và confirmed opposite valid shape6ms, error cùng phía>=1pitch.
  Giữ pending/last-side mới; invalid flicker hoặc TURN retarget không renew.
- Tăng extraP ngay frame đầu shape bằng ramp0,04/ms, cùng trần0,6. Rollback
  raw-trend early release B29; dùng planner gate B28. Confirmed-loss arc12ms,
  unknown-loss28ms. Không straight boost hoặc BRAKE trước cua; valid giữ PD.
- Tradeoff: first-frame cue có thể giả, rút arc có thể cần pivot sớm; một hàng
  sensor không chứng minh đường parabol hay nhận đúng nhánh. Vẫn giữ deadline
  thật, watchdog/Stop/Max/driver/calibration và Flash/CSV schema.
- Evidence:104000ARM+26Python tests pass, actual ARM command comparison so
  B28/B29/B30, Keil0errors/warnings; không có dynamics/lap prediction.
  Xem [B30](corner-chain-b30.md), ERR-052–054. BackupB29; chưa hardware-verified.

## ADR-041 — Rolling corner có phạm vi và fallback, không tăng Base (B31)

- Context: owner xác nhận B30 ổn định chuỗi góc nhưng chậm hơn B28 16,3s;
  yêu cầu hai bánh tiếp tục tiến để cua theo cung, tránh dừng bánh rồi xoay.
- Decision: giữ fix budget của B30; mixer của confirmed same-side corner/
  rolling rejoin dành floor cho bánh trong và dùng Max headroom cho bánh ngoài.
  Floor min(speed*0,4, Max*0,25); steering_limit<= (Max-inner)/2 trước antiwindup.
  Đây là ngoại lệ có phạm vi được owner yêu cầu, không là global inner floor.
- Initial confirmed rolling opportunity80ms/ratio0,6, unknown28ms/0,8; mọi
  valid frame lấy lại PD ngay. Retry không renew, pivot chỉ fallback khi rolling
  không thành công; toàn bộ deadline/Stop/watchdog/estimator giữ nguyên.
- Evidence: baseline B30 fail160030, actual ARM trace cho85/18 và rolling qua
  loss34ms thay vì pivot.105133ARM+26Python tests pass, Keil0errors/warnings.
  Không có plant, không suy ra quỹ đạo/thời gian vòng từ PWM.
- Tradeoff: mean duty có thể cao hơn Base khi dùng headroom, cung có thể rộng
  hơn và đi xa nếu cue sai. Owner chưa đo minimum running PWM;18% thử nghiệm.
  Không có encoder/IMU để bảo đảm vận tốc/góc, vẫn phải thử xe. Xem
  [B31](rolling-turn-b31.md), ERR-055/056, backup actual B30 chưa commit.

## ADR-042 — Công nhận sustained valid tracking trước line timeout (B32)

- Context: B31 owner test lặp lại đều dừng khi xe còn trên line, không có lap
  hoàn tất. F2/R2; source fail170029 với valid±1500 giữ liên tục trong ALIGN.
- Decision: ưu tiên xử lý valid trước line-timeout; fresh valid chưa xóa budget.
  Continuous40ms valid inner line, abs(e)<=2pitch/mask&3C/no81, mới công nhận
  settled tracking và release budget, kể cả lệch±1,5. Invalid/edge ngắt evidence.
  Xóa arc_speed cũ khi settle, không cấp lại blind arc do reset timer.
- Giữ center12ms handoff và deadline qua glimpse; handoff moderate không resetD.
  Timeout vẫn áp dụng khi invalid nếu chưa earn sustained tracking; không
  auto-restart, không bỏ fault latch/watchdog. B31 mixer/rolling/gains45/85 giữ.
- Evidence: mirrored steady line, deadline boundary, dt1–4ms, interrupted valid,
  outeredge và fault latch tests; toàn suite controller/menu/driver/log pass,
  Keil0Error/0Warning. Chi tiết [B32](recovery-exit-b32.md), ERR-057/058.
- Tradeoff: valid sensor không xác nhận đúng nhánh, LIVE error sau Stop không
  dựng lại frame lỗi. Chưa có CSV/hardware test B32; cần chạy lại đoạn cuối và
  hoàn tất map trước khi đánh giá tốc độ. Backup actual B31 trước sửa.

## ADR-043 — Phục hồi B28 controller core thay vì tiếp tục floor/arc dài (B33)

- Context: owner báo B32 hoàn tất17,4s, cua mềm văng, so mốc B28 16,3s. Không
  có CSV mới để định lượng phần mất thời gian. Hướng thử B31/B32 chưa đạt mục tiêu.
- Decision: phục hồi byte-exact Follow/TurnOutput từ B28: confirmed6ms gain,
  no inner floor/common boost, arc28ms/ratio0,8. Giữ12/0/0,12 và45/85. Chưa thêm
  một biến thể PID hoặc tăng Base; test trước khi tiếp tục chỉnh.
- Giữ fixes B30 chain budget và B32 valid±1500/valid-before-line-timeout, không
  rollback toàn repo về B28 rồi mang lỗi đã xác nhận trở lại. Menu/driver/Flash,
  estimator/DMA/watchdog/calibration/Start không đổi.
- Evidence: baseline B32 fail180028;104188ARM+26Python tests, Keil0errors/warnings.
  Hai64-frame traces pureP và defaultPD khớpB28, source hai hàm byte-equal.
  Historical floor/80ms tests thay trong default suite bằng restore-policy tests.
- Limits: khớp input/state synthetic không đo quỹ đạo hay lap. Bánh trong có thể
  rất thấp/0 khi cần bẻ mạnh; pivot vẫn cứu sau loss. Không hứa16,3s trước thử xe.
  Chi tiết [B33](b28-restore-b33.md), ERR-059, backup actual uncommittedB32.

## ADR-044 — Khôi phục đầy đủ B28 gốc theo lựa chọn owner

- Owner báo B33 khoảng17s, chọn B28 mốc16,3s sau khi ban đầu ghiB24. Không
  gán nhầm thành tích choB24; giữ riêng cả hai original source/HEX và manifest.
- Restore toàn bộ snapshot B28 vào active source/build/test, không ghép B30/B32.
  B33 có source/HEX backup trước thay đổi, không xóa historical files đời sau.
- Build mới khớp SHA256 HEX gốc B28,0errors/warnings;104022ARM+26Python pass.
  Chưa nạp board hoặc đo lại thành tích. B28 known recovery limitations vẫn có.
- Giữ error/decision history, đánh dấu current đã restore để tránh suy luận
  firmware còn chứa fixes đời sau. Chưa chỉnh tăng tốc hoặc corner policy mới.
  Xem [baseline B28](original-b28-baseline.md).

## ADR-045 — Đổi estimator thay vì tiếp tục đổi luật motor (B34 trial)

- Context: owner xác nhận B28 restore~17s ở45/85/PID12/0/0,12 và muốn hướng
  mới. Pre-edit source/HEX khớp original; không có log của lap này để quy nguyên
  nhân chênh lệch. Dừng ghép thay đổi gain/arc/floor như các lượt trước.
- Decision: chỉ estimator centroid của one strong group dùng connected support
  và weak shoulders; bỏ disconnected sub-threshold islands khỏi position.
  Mask/validity/strength/normalization không đổi, không select multigroup branch.
  Controller/robot/driver/ADC/Flash giữ byte-exact B28, tagB34 để audit.
- Evidence: baseline fail190035, C replay2 changed raw frames với expected
  position cụ thể, audit CRC2logs có6/303 valid frames thay đổi;105365ARM+26Python
  pass, Keil0errors/warnings. Không có ground truth/vận tốc/lap prediction.
- Tradeoff: weak island có thể là nhánh thật nhìn trước, không phải noise;
  bỏ nó có thể làm cua muộn. Giữ B28 HEX gốc và yêu cầu A/B thực tế, không gọi
  candidate tốt hơn từ test pass. Known B28 recovery issues vẫn còn.
- Chi tiết [B34](local-centroid-b34.md), ERR-061; backup B28-original-before-B34.

## ADR-046 — Direct recovery chung cho góc đầu và chained retry (B35)

- Owner yêu cầu giảm tối đa hai bánh tiến thêm hoặc rẽ luôn ở mọi góc, áp dụng
  phản ứng chained turn cho góc sau thẳng. Đổi policy thay vì đổi gains/estimator.
- BeginRecovery luôn arc_speed0; TurnOutput bỏ nhánh both-forward28ms. Chỉ
  accepted loss có heading/confirmation mới TURN; valid giữ forward PD. GAP,
  split6ms/side memory và1200msdeadline không đổi. Không invent direction.
- Duty50/36 clippedMax, driver reversal guard giữ. Rejoin dùng cap40 của pivot
  branch hiện có; không kèm tăng rejoin speed hoặc min-spin/geometry advance.
- B34 fail200030,105329ARM+26Python tests pass, Keil0errors/warnings; gap-memory
  tests được giữ khi thay historical arc tests bằng immediate-entry coverage.
- Quán tính thật chưa đo, lệnhcommon0 không có nghĩa không tiến thêm. Chưa có
  lap B35; biết nguy cơ giật/quét lố, giữ backupB34. Xem [B35](immediate-corner-b35.md), ERR-062.

## ADR-047 — Selective corner P và evidence-based fast handoff (B36 trial)

- Owner yêu cầu sửa theo video mẫu: đổi hướng gọn và tiếp tục nhanh. Không có
  source/telemetry của xe mẫu nên chỉ chọn mục tiêu hành vi, không suy gains.
- Tăng scheduled-extra ceiling0,6→1,0, giữ confirm6ms và ramp0,04/ms; nền12/0/
  0,12 và45/85 không đổi. Không correction floor hoặc valid forced pivot.
- Narrow central line08/10/18,abs(e)<=0,6,no pending, liên tục6ms mới cho fast
  handoff và targetBase theo ramp cũ; giữ D. Không reset direction/budget, invalid
  hoặc error lớn/corner mới xóa flag. Legacy handoff/cap còn là fallback.
- Evidence: baseline fail210028;94standalone+105332integration ARM+26Python,
  Keil0errors/warnings. Tests riêng executable để giữ map/budget emulator cũ.
- Tradeoff: cue giả có thể bẻ quá và center6ms có thể vẫn là nhánh cũ. Video
  không xác nhận chiều motor hoặc control mode; chưa có lap B36. BackupB35,
  chi tiết [B36](video-corner-b36.md), ERR-063. Known recovery bugs không đổi.

## ADR-048 — Owner high-Kd profile, scoped applied-D và calm exit ramp (B37)

- Owner báo16,11s ở55/85/12/0/Kd0,8..1,0 nhưng giật hơn ởchuỗi góc. Chọn0,9
  làm default thử, không giả định đây là exact best lap hoặc có thể bảo đảm15s.
- HighKd>=0,5 tại near narrow-center hoặc corner context: appliedD cap24 và
  rise1600pp/s, release giảm/sign cũ ngay; không slewP, không đổi rawD/planner
  filter, gain-corner/pivot/estimator. Bảo vệ tâm line tiếp tục sau post-turn.
- Earned fast exit chỉ ramp240 khi rawDterm<=8, inner narrow line/e<=0,8 và
  no pending. Không tăng startup hay tốc độ trên invalid; giữ Base55/Max85.
- Evidence: B36 fail220032; scriptedsameprofile jitterpeak42,5→9,2 và calm
  exit request43→55 đạt50ms (old chưa tới trong128ms). Không có dynamics/lap
  prediction.4955new+94corner+105327integration ARM+26Python pass, Keil0errors/
  warnings. Historical test profiles pinned explicitly, product/menu metadata
  defaults vẫn được test. Separate executable giữ emulator bounds.
- Rủi ro: D cap/ramp có thể làm giảm damping cần thiết hoặc đổi phản ứng tại
  scope boundaries; P/retarget vẫn có thể gây giật. Cần A/B B36 cùngprofile,
  không so với45/0,12 rồi quy lợi ích cho algorithm. BackupB36,
  chi tiết [B37](high-kd-b37.md), ERR-064/065; known timeout issues chưa đổi.

## ADR-049 — Valid authority, evidence để tách recovery và trợ lực góc sớm (B38)

- Owner báo B37 dừng lặp lại ở góc thứ ba, cung cấp source Car_Line_2026 và
  muốn học cách cua gọn trong video. Ảnh F2/R2 và baseline fail230023 xác nhận
  defect deadline-before-valid trong source; ảnh không cung cấp toàn bộ trace.
- Ưu tiên valid PD, giữ1200ms cho invalid; chỉ inner-line evidence40ms mới
  giải phóng budget cũ và moderate ALIGN. Completed handoff/valid travel24ms/
  confirmed opposite shape6ms mới coi là góc mới; không cấp budget từ glimpse.
- Extra-P bắt đầu từ shape đầu theo cùng ramp/gate, memory giữ6ms. Giữ55/85,
  12/0/0,9, B37 D shaping, direct loss pivot, estimator/ADC/driver/Flash.
- Không sao chép detector/gate/phanh/đảo bánh của reference thành một khối:
  chúng khác units và policy; white-clear từng gây regression trên xe của ta.
  Source mới còn giảm Base theo error, nên không chứng minh giữ nguyên tốc độ
  qua mọi góc. Video không cho biết PWM hoặc chính source xe mẫu.
- Evidence:541 new +94 corner +4955 highKd +105331 integration ARM,26 Python;
  Keil0errors/warnings. BackupB37, chi tiết tại car-line-b38.md/ERR-066/067.
  Source-fixed không phải hardware/lap-verified; không hứa15s từ emulator.

## ADR-050 — Signed reserve theo visible corner, không chạy waypoint bằng timer (B39)

- Owner báo B38 chưa cải thiện; lowBase bẻ kịp nhưng45/55 overshoot~2cm. Chọn
  sửa output saturation85/0 và mất assist khi cue thu hẹp, giữ Base/PID và các
  phần soft/GAP để tách ảnh hưởng. Không tăng gain khi correction đã chạm trần.
- Reserve tới15% inner reverse chỉ với confirmed visible side, duty/error
  weights, raw-return và rate gates; P/D chọn correction thực. Hướng tìm line
  khác với quyền signed steering; new single eye không cấp lại quyền này.
- Hủy reserve trên inward step rõ, dung sai nhỏ riêng cho eligibility tránh
  command chatter; giữ driver reversal guard và không có both-wheel dwell.
  Không khẳng định PWM−15 đã là reverse motor hoặc kinematics đo được.
- Map dùng làm reference: xe không encoder/track width chưa đo; không áp2-run
  odometry/timer/ESP32 theo template. Simulation không có inertia/slip model,
  metadata/path length chưa khớp nên không dùng để suy lap hoặc vị trí runtime.
- Evidence: baseline fail240031;1840 yaw+541recovery+4955highKd+94corner+
  106137integration ARM,26Python;32traces cùnggain/B38archive. Keil0errors/
  warnings. BackupB38. Cần A/B cùngsetting và video/CSV trên map trước khi
  kết luận hết2cm hoặc nhanh hơn. Xem B39,ERR-068/069 và reference README.
## ADR-051 — B40 ưu tiên góc nhìn thấy trước recovery

Theo CORNER log B39, sửa derivative reset qua nonwhite PROBE ngắn và bổ sung
proportional branch bias có xác nhận. Không tăng Base/Kd hay lực search để
che thời gian vào cua muộn. Giữ mọi giới hạn recovery/driver và dữ liệu Flash.
Chi tiết evidence, luật và validation: [B40](branch-entry-b40.md).

## ADR-052 — B42 tách active-eye mixer khỏi centroid và recovery bookkeeping

Owner yêu cầu thực hiện hướng EYB sau review, thay cơ chế hỗ trợ visible corner
B36–B41. Chọn strengths/hysteresis để lấy cue tức thời, dedicated corner ceiling,
P-before-D reverse assist và common recovery ramp. Tách vị trí liên tục khỏi
observation tách cụm có hướng; không coi mọi multi-group là line đáng tin.

Giữ hardware/ADC2ms/menu/driver guards/watchdogs/GAP và bounded recovery. ALIGN
không thêm drive cap cho valid line. Ki0, defaultKd0,10/filter4ms là trial mới;
không dùng gain0,9 của luật cũ hoặc suy ra gain từ số mắt đơn thuần.
Blackbox giữ layout, thêm active-eye rank vào flags theo firmware tagB42.
Backup B41 đầy đủ trước sửa. Evidence và validation: [B42](active-eye-b42.md).

## ADR-053 — B43 quay bất đối xứng trên nền B41

Theo authorization owner, bỏ active-eye trial B42 và khôi phục B41. Thay
recovery counter-rotation bằng64/−22 khi còn signal và68/−30 khi trắng, mirror
theo hướng. Giữ carry tối đa48 (không boost low duty), rejoin cap52, rút qualification
góc tiếp8ms và center clear40ms. D10ms/cap28/reverseTRACK24 là trial yêu cầu.
Giữ Max, GAP, antiwindup, driver guard và bounded recovery/valid-before-timeout.
Source/replay chỉ xác nhận command step nhỏ hơn, không đo giảm quán tính hoặc
lap trên xe. Backup B42/source checks và verification: [B43](swing-turn-b43.md).
