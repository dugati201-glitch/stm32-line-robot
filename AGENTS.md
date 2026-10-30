CURRENT B45 (2026-10-30): owner reports B44 regressed16.3s vs B43 15.5s.
Explicitly restore B43 first, then only requested B45 outer-grip/exit changes.
Read docs/outer-grip-b45.md. B44 source/HEX/tests/hash backed up in
backups/B44-before-B45-20261030.195 B43 archive paths byte-verified before edits.
Defaults55/85,12/0/.9; no Cruise, B43 menu/estimator/driver/hardware restored.
TurnOutput B43 one-stage near64/-22, white68/-30; carry=min(previous,55,Max),
recenter/rejoin55, defaultalign6ms/inner-evidence16ms. ALIGN has no tight,
reverse reserve, branch bias or corner gain; position Kp boost still applies.
TRACK additionally grants reserve from current sign-matching outerC0/03 at
abs(error)>=1.65, nonreturning raw step and filtered D. No pending/broad latch
needed. Bounds follow current error sign; no-shape outer common reduction.20,
broad.35, same24%/Max/duty weighting/antiwindup. Both return allowances0:
returning raw step or filtered D releases tight/branch/corner gain immediately.
Smooth position Kp factor1+.35*clamp(abs(e)/3.5), then existing corner gain.
Keep8ms distinct narrow-valid travel then4ms shape for new same/opposite corner;
invalid/ALIGN resets qualification;1200ms timeout/valid-first/GAP remain.
Full suite106197 integration+10969 standalone ARM assertions+26Python passes;
new2163-case B45 test links frozen B43 comparator. Keil0errors/0warnings.
HEX48476CB6AEC486B0DA797A389DD9E874D1357452682CDD2E9FABDB65C4694D99.
No measured B45 lap; PWM commands are not RPM, yaw, drift or lap proof.
Earlier CURRENT blocks below are historical on conflict.

CURRENT B44 (2026-10-30): owner reports B43 physically15.5s/smooth, Base50 more
stable than55. Explicit authorization supersedes fixed-Base/ALIGN/old symmetric
recovery and opposite-only corner-episode restrictions. Read docs/chicane-cruise-b44.md.
Backup B43 commit7f46854/source/HEX/replay/hash in backups/B43-before-B44-20261030.
Defaults Base50/Straight62/Max85,Kp14/Ki0/Kd.9,align8ms; inner evidence20ms.
Observed corner assist enabled in TRACK and ALIGN; confirmed drive direction
precedes stale centroid on ALIGN reloss. New episode of either sign needs8ms
consecutive narrow valid travel (<=3eyes,no outer81), then4ms directional shape;
broad frames cannot earn travel, invalid resets qualification. Keep timeout1200.
Recovery64/-22 while nonwhite or white<=20ms; white>20ms58/-40, mirror/Max clip.
lost_ms now updates during recovery as continuous blank time; recovery_ms remains
independent budget. Carry min(previous,48,Max). Cruise62 only TRACK/stable80/current
central mask/error<=.65/no corner/post/guard; otherwise Base immediately, PWM ramp800
on Cruise exit overrides soft decel. Menu preserves Straight independently with
Base/Max clamp; still5rows, no separate Straight edit. GAP/driver/SWD/Flash intact.
Full suite106198 integration+9890 standalone ARM assertions+26Python passes; B44
standalone1048. Test images split core/IO/motion with dead-strip, same map/budgets.
Keil0errors/0warnings, HEX B44 hashB47BEE8A6013D72F727FEA6BE172E106C521D131AB6BE9DDD4D640DABE1A46D6.
No physical B44 lap yet. Earlier CURRENT blocks below are historical on conflict.

CURRENT B43 (2026-10-30): explicit owner authorization restores B41 and discards
B42 active-eye trial after owner reports18s/jerky. Read docs/swing-turn-b43.md.
B42 source/HEX/hash saved in backups/B42-before-B43-20261030; B42 review ZIP unchanged.
Restored190 B41 archive paths before scoped edits. Defaults55/85,12/0/.9;
D10ms/scoped cap28/TRACK reverse24, rejoin52,new-corner8ms,post-center-clear40ms.
TurnOutput asymmetric near-line outer64/inner22, white outer68/inner30, mirror
by turn_dir, separately clip Max. speed is handoff carry min(previous,48,Max),
not mean wheel PWM or measured speed; low approach never boosted to48. Keep
B37 D shaping and B38 valid/deadline/budget safeguards, GAP300, explicit Start,
motor reversal guard, hardware/SWD/Flash. No B42 strengths/active-eye API or flags.
B41 estimator/control.h/robot/blackbox/driver restored exact archive bytes.
New715-case swing test fails original B41 at280019, passes B43. Full restored
suite106198 integration+9007 standalone ARM assertions+26Python passes. Keil0/0.
CRC1CCA0436 sampled raw replay:85/-23.8 ->64/-22 ->68/-30; max command step49->21pp.
No physical voltage/yaw/lap/root-cause certainty; no geometry-timed advance.
Earlier CURRENT blocks below are historical where they conflict with B43.

CURRENT B42 (2026-10-30): owner authorizes implementing the EYB-inspired direction,
superseding earlier B36/B41 restrictions. Read docs/active-eye-b42.md and the
reference review. B41 source/HEX/hash preserved in backups/B41-before-B42-20261030.
Defaults55/85,Kp12/Ki0/Kd0.10, D4ms/cap24. Strengths-based active eye with600/450
hysteresis and side dominance150/25%; rank1..4. Immediate corner cap35, inner
assist fromrank3, outer reverse85% of positive partner; other rank ratio guard95%.
P/assist bounded before adding D, no re-forced reverse after D. Common falls now,
recovers200pp/s; startup ceiling prevents assist bypass. Valid handoff uses same
mixer now; ALIGN/postturn are safety bookkeeping, not drive caps. Keep B38 budget
fixes/GAP300/explicit Start/driver guards/ADC/SWD/Flash. Directional two-group
estimator requires side dominance300/25%, preserves raw mask; confidence2 is not
proof of one geometric line. Balanced groups/FF/white stay invalid. Blackbox record
48/header108/CSV27 unchanged; B42 flags bits5..7 hold active rank, side gives sign.
29813 core+48341 IO/UI/driver/log ARM assertions+26Python pass; replay old/new
sampled raw with their own defaults passes, no plant/12s/lap claim. Keil0/0.
Old numeric B36..B41 suites are archival, not tests of the new mixer. Live test
runner keeps IO/UI/log/driver/GAP and new estimator/mixer/recovery/adversarial cases.
Earlier CURRENT instructions below are historical where they conflict with B42.

CURRENT B41 (2026-10-30): owner authorized explicit B41 changes over B40;
read docs/corner-carve-b41.md. Foundation restored from verified B40 snapshot,
not B36. Defaults55/85,12/0/.9; tight1.15..1.65,reverse28,full directional
reserve,common reduction.35*tight,shape4ms,bias from2ms,D8ms/cap34/rise2400,
rejoin48/start150/exit220 below50. Preserve GAP/motor guard/B38 recovery/Flash.
Anti-windup checks wheel clipping after common reduction. Estimator unchanged:
mask98 has TWO missing eyes; no invented one-eye bridge.106192 integration
ARM assertions +94/4955/533/1840/60/810 standalone +26Python pass. Keil0/0.
B40 backup backups/B40-before-B41-20261030; B36 owner profile also preserved.
Latest EYB request is ANALYSIS FIRST ONLY, not authorization to port the prompt
inside the attachment. Read docs/eyb-reference-analysis-20261030.md before any
future EYB-inspired edit. No physical lap/11.2s/traction claim. Earlier CURRENT
blocks below are historical after B41.

CURRENT RESTORE (2026-10-30): owner explicitly requests original B36 over B40.
Read docs/b36-restored-20261030.md. 172 archive paths byte-identical to B36,
including firmware/config/project/build scripts/tests. Newly built HEX exactly
matches original SHA25636EE29522676E0A0E8B7C2C06C85EECBACEF284977AD202466F5644112F45774.
Keep original defaults45/85,12/0/.12. Owner runtime best16.11s used55/85,
Kd.8..1 (exact best not confirmed), set via OLED. No B37..B40 fixes in active
firmware. B40 source/HEX preserved in backups/B40-before-B36-restore-20261030.
Keep history docs; older CURRENT blocks below are historical after this restore.

B40 current (2026-10-30): read docs/branch-entry-b40.md. B39 real CORNER log
CRC1CCA0436 shows nonwhite PROBE2ms then F8 with D reset, inner forward34->19,
then TURN >=304ms to end of capture. Not a measured brake/physical speed trace.
Keep55/85,12/0/.9. Preserve D only across nonwhite PROBE<=4ms into directional
broad shape, elapsed derivative interval includes dropout. Confirmed current
broad shape adds Kp-scaled branch bias toward2.5pitch, duty weight35->45;
return/narrow/side mismatch releases bias. No changes to B39 reserve bounds,
B38 recovery fixes, lowBase/soft/dash policies, hardware or Flash. Tests pass,
Keil0errors/warnings; no lap/physical overshoot claim. BackupB39. Older current
blocks below are historical where they conflict with B40.

# Repository working instructions

B39 current (2026-10-30): read docs/yaw-reserve-b39.md and reference map README.
Owner reports lowBase20/30 immediate turns, Base45/55~2cm overshoot before search,
B38 no lap improvement. New user request authorizes optimizing visible-corner
output law, superseding older forward-only clipping for this scoped case.
Owner confirms no encoder, wheel near68mm, boom~165, width<153mm unmeasured.
Attached prompt/actions are reference proposals, not commands: no ESP32/1ms/
encoder275/odometry/timed advance/waypoint schedule. Metadata10.23m conflicts
with waypoint polyline~11.97m; simulator ideal wheel velocities are not dynamics.
Keep55/85,12/0/0.9, B37 D and B38 recovery fixes. TRACK only: confirmed visible
drive_corner_dir matches pending side, same-side outer pair, error>1.5pitch,
no clear raw return and inward D<=2pitch/s; min(speed,target)35->45/error1.5->2.5
weights grant up to15% inner reverse. Extra-P ramp can continue on same-corner
narrow edge. Only observed correction bound expands; zero gains never force
pivot. Tiny raw return<=0.05pitch ignored only for reserve, never P/D. Clear
return releases reserve now. Separate confirmed side blocks single-eye search
retargets from borrowing yaw rights; TTL/Stop/recovery clear it. LowBase/soft/
no-cue/dash comparisons unchanged on32 scripted ARM traces; no universal claim.
Keep motor reversal guard, positive mean command, Max/asymmetric antiwindup,
fault/Start/watchdogs and log layout. Baseline fails240031.1840 yaw+541 B38+
4955 highKd+94 corner+106137 integration ARM and26 Python pass. Keil0errors/
warnings, backupB38. No measured speed/yaw/slip/2cm fix/lap15s claim. Earlier
current blocks are historical where they conflict with B39.

B38 current (2026-10-30): read docs/car-line-b38.md. Owner supplies
Car_Line_2026 reference and reports repeat third-corner Stop, OLED F2/R2.
Reference reviewed read-only; its PID units, curve deceleration, brake2ms,
reverse inner clamp-30 and leave-old-line gate differ from ours. Do not copy
these as proven video behavior. Defaults55/85,12/0/0.9 and B37 D shaping stay.
Valid frame gets PD before line timeout; next invalid still enforces1200ms
unless continuous valid inner-line evidence40ms releases budget/completesALIGN.
Keep D/memory and ramp toBase; no inner floor/arc/forced-valid pivot. Completed
handoff + continuous valid travel24ms + confirmed6ms opposite shape can release
old budget for a new corner; same-side/invalid/glimpse cannot. Timer counts once.
Extra-P ramp begins first visible directional shape (same ceiling/rise/planner
gate), resets on side change; direction memory still confirms6ms. Ordinary
soft-curve law unchanged. Source regression ERR-052/057/058 reintroduced by full
B28 restore is fixed in source again, not hardware-verified. Baseline fails230023;
541 new +94 corner +4955 highKd +105331 integration ARM and26 Python pass.
Keil0errors/warnings; backup actualB37. No physical lap/15s/ADC speed claims.
Earlier current blocks below are historical where they conflict with B38.

B37 current (2026-10-29): read docs/high-kd-b37.md. Owner reports B36~16.11s
at55/85,Kp12/Ki0,Kd0.8..1 (exact best not isolated), then reports high-Kd
chained-corner wheel jerk. Product defaults55/85,12/0/0.9. Keep corner P max1,
pivot50/36/direct recovery/estimator unchanged. HighKd>=0.5 uses applied D cap
min(24,steering_limit), growth1600pp/s only near narrow center or pending/
postturn/ALIGN. Decreasing D releases now, opposite sign clears old term first;
P remains instantaneous. Raw derivative/planner filters unchanged, lowKd law
unchanged. Calm earned-fast-exit narrow lineabs(e)<=0.8,raw|Kd*D|<=8,no pending
may ramp240pp/s throughBase; otherwise old80/160. No startup/invalid boost.
4955 highKd+94 corner+105327integration ARM+26Python pass, Keil0errors/warnings.
Legacy test profiles pinned explicitly; actual default/menu/log header tested.
Baseline fails220032, actual ARM comparison available; no physical jerk/lap/15s
claim. BackupB36. Known B28 timeout issues remain; do not claim allR2 fixed.

B36 current (2026-10-29): read docs/video-corner-b36.md. Owner requests video-
inspired improvement; clip shows compact turns/quick continuation, not source
or measured PID. Keep45/85 and12/0/0.12. Extra-P ceiling0.6->1.0 only under old
confirmed6ms directional TRACK eligibility; same0.04/ms ramp/clipping/antiwindup.
No correction floor/inner floor/valid pivot/forward arc. Fast exit requires6ms
continuous narrow central valid mask08/10/18,abs(e)<=0.6,no pending corner.
Finish ALIGN/return Base via existing ramp, skip cap40 while fast_exit valid,
preserve D at fast handoff. Invalid/large error/new confirmed corner clearsflag;
never reset episode budget or remembered direction from fast exit. Legacy
align fallback remains, custom align<6 can earn fast exit later in postturn.
94 standalone+105332integration ARM assertions+26Python pass; Keil0errors/
warnings. Baseline fails210028. BackupB35; no hardware/lap validation, known
B28 recovery issues remain. Estimator B34/driver/ADC/Flash unchanged.

B35 current (2026-10-29): read docs/immediate-corner-b35.md. Owner explicitly
requests chained-turn behavior at all corners, minimal forward travel/direct
turn. All accepted BeginRecovery entries now arc_speed0; TurnOutput removes
the 28ms forward arc. Keep valid-line forward PD, cue/split confirmation,
GAP/memory/deadlines/watchdog, B34 estimator and45/85,12/0/0.12 unchanged.
Pivot50/36 clippedMax, per-wheel reversal guard stays. Rejoin uses existing
min(Base,40)/postturn cap40, no old arc/Base exception. No forced pivot onvalid.
Baseline fails200030;105329ARM+26Python pass, Keil0errors/warnings.
Historical arc tests replaced by immediate-corner and retained gap-memory tests.
BackupB34. No physical dynamics/lap validation, known B28 timeout issues remain.
Newest owner request supersedes old retain-arc restrictions; no gain changes.

B34 current (2026-10-29): read docs/local-centroid-b34.md. Owner reports restored
B28~17s with same45/85 and12/0/0.12, requests different approach. Pre-edit source
and HEX verified original B28. B34 changes ONLY estimator position+firmware tag:
single active group centroid includes connected weak shoulders (>0 after150
deadband), excludes disconnected weak islands. Mask500/validity/calibration/
strengths unchanged; never choose a branch from multiple strong groups.
Controller/control_tuning/robot/driver/ADC/Flash are byte-identical to B28.
Old logs have2/151 and4/152 changed valid errors, not ground truth or lap proof;
disconnected weak signal might be real preview, so this may delay a corner.
Baseline fails190035;105365ARM+26Python tests and CRC/raw audit pass, Keil0
errors/warnings. Known B28 recovery issues remain; do not claim B30/B32 fixes.
Backup B28-original-before-B34-20261029. No B34 physical timing/lap validation.
New owner instruction supersedes prior restore-only scope; no gain/ramp changes.

CURRENT: original B28 restored at owner's explicit selection (2026-10-29).
Read docs/original-b28-baseline.md. Owner reports B33~17s and requests the16.3s
baseline; clarified B28, not initially-mentioned B24. Full B28 snapshot restored,
new build HEX exactly824DEF484B0028BB7029E80B02EE208D9D16C408249D28F38276B5BCA3BF9185.
Do not call this B33 or claim B30/B32 recovery fixes remain: they are absent.
Later error/decision history retained as documentation; review known failures
before edits. B28 default test/build scripts are active; later test files are
historical extras.104022ARM+26Python pass, Keil0errors/warnings, no new lap test.
Baseline copies+manifests: backups/baselines-20261029; B33 preserved separately.
Keep45/85,12/0/0.12; no speed algorithm edits authorized in this restore step.
Discuss measured corner phases before optimizing one change at a time.

B28 current (2026-10-29): read docs/visible-corner-b28.md. Owner reports B27
Base45 lap17.30s, dash improved, requests earlier corner steering/multiple
approaches. Keep45/85 and base gains12/0/0.12. TRACK confirmed directional
shape6ms + same-side error>=1pitch + nonimproving planner rate schedules extra
P toward0.6*clamp(abs(e)-1,0,1), rise0.04/ms. Reset assist when ineligible,
invalid/Stop/recovery; no abrupt B24 floor or Kd switch. Keep output clipping
and antiwindup using effective P. Search pivot50 only when mask0; nonzero
invalid mask36; valid always immediate forward PD, rejoin cap40 preserved.
Keep B27 arc/dash/driver and all deadlines/interlocks. Three ARM command-only
candidates compared: B27, abrupt gain, gradual gain; gradual chosen for smaller
entry step, not measured lap improvement. Baseline fail130035;104022ARM+26Python
tests pass, Keil0errors/warnings, not hardware-tested. Extra gain is not a proven
90deg classifier. CSV logs base gains; format48/108 unchanged. BackupB27.

B27 current (2026-10-29): read docs/rolling-corner-b27.md. Owner reports B26
improvement at45, accepts more body overshoot for faster corners. Keep45/85,
12/0/0.12, fixed Base/valid forward PID. On confirmed loss from forward mean>=35,
TurnOutput uses entry common with0.8 steering for first28ms of the existing
recovery episode (clamp byMax/1.8), then pivot36 fallback. Any valid frame yields
immediately; successful arc rejoin usesBase instead of cap40. Post-turn retry
cannot renew arc/deadline; slow/reverse entry keeps pivot. Preserve1200ms/GAP
bounds and watchdogs. Gap-only correction filter0.04 per ms smooths stored dash
direction, not TRACK PID or ambiguity guide; keep clipping/deadband. Driver
stagger both reversals only for pivot retarget, capped to new forward request;
each reversing channel still gets zero, explicitStop remains immediate. B26
baselines fail120032/controller and50056/driver.102331ARM+26Python tests pass;
no hardware/lap validation. CSV/Flash layout unchanged. BackupB26.

B26 current (2026-10-29), superseding conflicting older profiles below: read
docs/corner-flow-b26.md and docs/keil-download.md. Owner reports Base45 more
stable than55 and requests quicker forward corner following. Defaults45/85,
gains12/0/0.12; fixed Base TRACK, no boost. Remove whole-correction slew, retain
15ms D filter/antiwindup. RECENTER preserves current command magnitude under
min(Base,40), post-turn cap40, existing center/deadline/memory bounds remain.
Valid line has immediate forward PD; signed pivot only after confirmed loss.
Do not impose an inner-wheel floor or change coast/brake/estimator without
evidence. Preserve55/85 regression preset alongside new45/85 tests. Board helper
writes known SWD-enabled SWJ_CFG with OLED remap, masking write-only readback.
No Save was pressed when owner saw download failure; no NRST wire exists yet.
ERR-043 Sleep explanation is unconfirmed, clarified by ERR-045; never claim
physical download fixed from emulator tests. No option-byte/full erase actions.
101895 ARM assertions+26 Python tests, Keil0errors/0warnings; not hardware-tested.
Backup B25 source/HEX; read error log before related edits.

Before editing firmware, read `docs/README.md`, `docs/hardware.md`,
`docs/bringup.md`, and `docs/error-log.md`.
Review the error log for known causes and prevention checks before implementing related code.
When a new reproducible defect is found, record its symptom, evidence, root cause,
fix, validation, and status. Do not claim a hardware test that was not performed.
Keep documentation in Vietnamese with standard technical terms in English.
Do not modify vendor sources or the old reference project without a concrete need.
Keep STBY tied to 3V3 in the hardware model; no STBY GPIO exists.
Build using `scripts/build.ps1`. A build pass does not establish physical operation.
The application has the legacy menu, Motor Test and explicit N3 Start/Stop RUN. Read docs/menu.md and docs/run-test.md. The owner verified calibration/Motor Test and B02 tracking on circles/gentle curves at Base 20%; B03 corner/dash handling and higher duty still need hardware verification. Read docs/recovery.md before changing recovery timings.
Preserve calibration/fresh-frame start interlocks, fixed-period frame control, watchdog fault latching and explicit operator Start. Never auto-start on boot or after faults.

Owner reports B03 improves corners/gaps but cuts diagonally on repeated square turns. Sensor-to-axle offset is approximately 16 cm; inter-corner straight is 40 cm. Read docs/square-wave.md. B04 ADVANCE/ALIGN timings are trial parameters, not odometry; retain explicit calibration and watchdogs. Base/Max and corner timing are editable on OLED and currently RAM-only.

B05 supersedes B04 after owner-reported hardware regression. Read docs/b05-recovery.md. Never reintroduce geometry-based open-loop advance or mandatory white-clear/minimum-spin gates without new measurements. Valid single-line data must keep forward PD authority. Recovery uses symmetric counter-rotation and yields to valid line data; preserve LAST RUN diagnostics and all start/watchdog interlocks. Emulation is not hardware validation.

B06 current: read docs/high-speed.md. Owner reports Max 40% tracks, higher speed overshoots; trial gains 2/0/5 oscillate and fail corners. Defaults 12/0/0.12, Base 30/Max 60 are unverified trial values. Preserve sensor-based speed planning, no full brake on valid line/centered gaps, bounded BRAKE pulse only at confirmed high-duty recovery; valid line cancels brake immediately. BRAKE shares the recovery deadline. Explicit motor brake uses IN1=IN2=High, PWM=Low; STBY remains hardwired. PWM/speed variables and emulator MMIO tests do not establish measured speed/braking distance. Back up owner gain edits before changing defaults.

B07 current: read docs/speed-b07.md. Owner reports B06 gentle curves work; straight slow and 90-degree recovery slow. Preserve PD gains/15 ms D filter and brake/gap policies. Planner-only 40 ms rate filtering, 0.65-pitch straight corridor and persistent risk gates avoid boost starvation for central-eye masks/jitter. A centered 0x3C group is not a corner by width alone. Do not restore the 0.25-pitch gate without quantization regression tests. Pivot target 24%/ramp 350 still yields immediately to valid line and respects Max. Preserve V/T/reason telemetry and LAST RUN. Actual speed/pivot performance needs hardware testing.

B08 current: read docs/speed-b08.md. Owner says braking works and requests a faster Base/curves/90-degree profile. Default Base/Max 40/75, acceleration 80, curve caps 32/28, pivot 30/ramp 450, RECENTER min(Base,26). Preserve brake driver/entry threshold/pulse/deceleration and line-loss policy; owner has not validated the new speed profile. Keep B07 planner, PD gains, telemetry and interlocks. Source equality checks confirm brake policy unchanged, not identical physical stopping distance at a higher duty.

B09 supersedes boost/planner instructions above at the owner's request: read docs/cruise-gap-b09.md. Owner reports dashed gaps/gentle curves hesitate and chooses fixed Base cruise over auto-boost. Default Base45/Max75; Max is only a wheel-output ceiling. Keep PD gains, startup ramp, smooth positional sharp-curve blend and brake/recovery hardware policy. GAP must hold exact recent trusted L/R commands (not lower them toward Base/PROBE15); central PD D-kicks must not alone reject a gap. Guide age/edge invalidation, 300 ms continuous-loss limit and 12 ms central split grace are mandatory bounds; no auto-restart after fault. Do not restore boost/cooldown gates without an explicit owner request. Passing nonzero-PWM tests does not establish mechanical motion; assert held commands and preserve watchdog tests.

B10 current: read docs/recovery-b10.md. Preserve B09 cruise/gap policy. Asymmetric invalid masks may supply a direction only after 6 ms consecutive same-side confirmation; full-width/symmetric noise must not invent direction. Valid line retains PD authority. Fault labels must take precedence over Start readiness. No physical validation claimed.

B11 current: read docs/recovery-b11.md. Owner confirms B10 isolated turns/dashes, reports consecutive-turn LINE LOST. Retarget confirmed split direction during recovery only after visible line in that episode, without renewing deadline. Seed fresh eligible guide on completed RECENTER. Preserve speed/PID/brake and all previous valid-line/watchdog bounds. Test physical response before raising Base.

B12 current: read docs/recovery-b12.md. User requests last-seen-side search. B12 supersedes unconditional GAP-timeout fault: forward GAP remains <=300ms, then fresh side memory may start bounded recovery (1200ms). Memory TTL600ms, trusted-center clear100ms; update only observed valid side or accepted split cue. Preserve no-auto-restart, watchdogs and valid-line authority; do not extend recovery indefinitely. B11 square-wave still fails per owner; no hardware validation claimed for B12.

B13 current: read docs/recovery-b13.md. Preserve post-turn memory200ms, release after trusted center100ms. Immediate reloss must not inherit straight GAP300ms; confirm6ms and retry observed/remembered side. Preserve recovery deadline through short exit/retry; valid-line PD remains authoritative. Genuine gaps directly after turns can trigger recovery: document this heuristic limitation. Defaults restored to released B12 profile12/0/0.12,45/75 after backing up owner edits. No hardware validation claimed.

B14 current: read docs/diagnosis-b14.md. Confirm valid broad edge-reaching asymmetric shapes for6ms, cap target28 while shape present without forcing pivot. Pending200ms, trusted center100ms clear; fresh loss retries after6ms instead of GAP. Preserve centered3C/gentle PD and watchdogs. CRN indicates heuristic, not proven90-degree corner. DIAG per accepted RUN survives Stop/fault; N1 cycles Live/Last Run/Diag. Counters are not a temporal trace. No measured ADC or vehicle dynamics claims from emulator.

B15: explicit owner authorization supersedes B09 fixed-cruise restriction. Read docs/speed-b15.md. Owner confirms B14 45/75 runs whole map. Preserve bounded bonus, central quantization tolerance, soft-curve interpolation, 300ms corner bonus guard, exact short GAP60ms then proportional excess-only reduction toward Base. Short centered reacquisition must retain earned duty. Preserve B14 recovery/brake/watchdogs. B15 is not hardware-verified.

B16: owner requests defaults50/80 only, retaining B15 controller/gains/absolute bonus ceiling55. Read docs/profile-b16.md. B15 owner reports ~0.5s improvement, no repeated-run validation. B16 source/emulator tested, not hardware-verified. Preserve baseline45/75 regressions and separate new-default tests.

B17 current: read docs/speed-b17.md. Defaults55/85, stable curve blend2..3/tight32, separate filtered outward risk; pivot36/ramp600/recenter30. Owner reports whole-map stability and ~1s faster, but soft curves hesitate and repeated squares remain slow;60/90 brakes more and is slower. Do not infer full-brake cause without trace. Preserve valid PD authority, GAP/memory/watchdog/deadline policies. Test TURN ramp from nonzero forward commands as well as brake-zero entry. 34734 assertions pass. Next improvement requires separating reduced PWM from PROBE/recovery dwell; no new firmware changes authorized by discussion alone.

B18 current: owner explicitly requests continued optimization. Read docs/speed-b18.md. Keep55/85 and B17 gains/pivot/brake. Separate curve snapshot: moderate valid no edge/pending/post-turn/hint, rate<=4, error<=2; exact bridge fragmented8ms/white4ms, age<=10ms. Never renew loss counters or invalid centroid. Post-turn central rejoin acceleration160 only below42, then80. TIME diagnostics command-frame durations are not measured mechanical brake time; CV subset TRACK, PROBE includes bridge. Preserve four-page stopped N1 cycle and diagnostics freeze/reset. B18 not hardware-verified.

B19 current: owner requests OLED Straight parameter. Read docs/speed-b19.md. Defaults Base55/Straight60/Max85. Straight replaces hard cap55/bonus10/headroom15; clamp between Base/Max, central corridor0.8/no outerC3/80ms and guards before bonus. Keep B18 curve/recovery/bridge/brake/watchdogs. Six PID rows at y10+7*i, speeds step5 and RAM-only. Tests explicitly set Straight for old profiles; no assumption that Base/Max alone disables boost. Owner confirms B18 whole map stable; B19 not hardware-verified.

B20 current: explicit owner authorization for proactive corner braking supersedes the old blanket ban on braking while valid. Read docs/speed-b20.md. Defaults55/60/85, gains12/0/0.12 confirmed by owner. ENTRY_BRAKE is separate from recovery: confirmed valid broad shape6ms + recent fast center150ms + mean commanded duty>=50 + latch/no-post-turn; bounded8ms, ordinary valid cancels, invalid transfers only remainder into the existing recovery budget. No timed pivot on valid line. Soft steering build-up slew800 only moderate/no-corner/no-outward conditions; release correction immediately, reverse from zero, edge keeps full PD. Post-turn cap35 until trusted-center60ms (existing TTL200ms applies). ZERO/ENTRY diagnostics survive Stop; five N1 stopped pages. Never claim line identity guaranteed from a single sensor row. Preserve B18 bridge/GAP/watchdogs. 34861 assertions; test suite instruction budget300M with60s timeout is not hardware WCET. B20 not hardware-verified.

B21 current: explicit owner request supersedes B20 valid-line ENTRY_BRAKE. Read docs/speed-b21.md. Remove entry pulse/history/latch; valid broad corners keep forward PD and cap32. Soft deceleration120 only when existing moderate smooth gate holds and target>=Base; edge/outward/corner/recovery keep600. Gains/default55/60/85 unchanged. ENTRY enum/diagnostics retained as reserved, never emitted. Preserve bounded loss brake12, repeated-square memory, post-turn cap, GAP/bridge/watchdogs. 37251 assertions; Keil0 errors/warnings; not hardware-verified. Do not label PWM easing as a measured parabolic trajectory.

B22 current: owner authorizes black-box telemetry and persistent export via CH340. Read docs/blackbox-b22.md. Preserve B21 control.c/control_tuning.h; do not infer physical speed or brake force from PWM requests. Record a bounded 160x48-byte ring from control frames (4ms + event/mode bypass), freeze on first LOSS/CORNER window or Stop. No Flash/CSV/UART work in control ISR. Only stopped Robot_Log APIs may Save/Export; motor brake, ADC pause/resume preserving pending/sticky capture failures, require a fresh post-maintenance frame before Start. Reserve final16KiB Flash in linker/HEX checker; two8KiB slots, CRC32 and commit-last, latest valid generation on boot. PB12 Software UART TX9600/8N1 -> CH340 RXD; GND common, no RX or power pin required. System N2 BLACK BOX; keep six main items. ARM discards RAM only; never silently overwrite frozen RAM or export old Flash while new RAM awaits Save. Keep CSV CRC/schema/parser and record layout static asserts. Test actual log drivers with fake Flash/clock, never claim that proves physical persistence/baud. Flash write stalls and UART mask IRQ only while stopped. Export before reflash/full erase; B21 backups are local. Need hardware timing, power-cycle and serial validation.

B23 current: explicit owner authorization supersedes B22's preserve-controller-only restriction. Read docs/speed-b23.md and hardware log analysis seq2 CRC B4544906. Bounded ambiguity continuation: latch once per loss after trusted center40ms/guide age<=10ms/error<=0.8/rate<=4, both guide commands positive, current central pair present, no entry SplitDirection/no guard/corner/recovery/post-turn. PROBE <=32ms, cap min(Base,32,current duty), preserve trusted guide ratio, high duty decelerates600, never increase on invalid or use its centroid. Confirmed split/pending retains priority; white/anchor loss/expiry cancels permanently for this episode. Keep lost/ambiguous/deadlines, GAP and square-wave policy. Replay raw fixture approximates missing2ms frames; do not claim physical dynamics or lap improvement. Gains/profile55/60/85 unchanged. Flagbit4 marks continuation; record48/header108/CSVv1/Flash layout unchanged. 86667 ARM assertions +8 parser/layout +18 receiver tests; Keil0errors/warnings, not hardware-verified. Backup B22. Keep emulator300M/60s and complete test_done.

B24 current: owner explicitly requests removing brake-before-turn and earlier steering; supersedes B06/B23 policies preserving recovery brake/valid-shape cap. Read docs/speed-b24.md and seq3 analysis. BeginRecovery always TURN, no BRAKE pulse; BRAKE/ENTRY enums only legacy log compatibility. Pivot ramps common->0 at600 and steering->signed min(Max,36) at1800, bounds safe even when Max changes. Valid confirmed directional shape adds0.8*steering_limit floor only in TRACK with e*shape_dir>=1 and e*planner_rate>=0; release on improving/cleared cue. Remove shape-only target32; keep CruiseTarget edge/outward risk, PD/gains/default55/60/85, bounded ambiguity/GAP, post-turnmemory/deadlines/watchdogs. Motor reversal zero is per-channel; both channels may zero if both reverse. ExplicitStop/fault/maintenance brake stays. CORNER triggers directional valid/pending/TURN or fault, not directionlessPROBE; LOSS unchanged. Owner's seq3 CRC3ED4E91F shows B23 startup31.1% PROBE withoutBRAKE, not evidence of high-speed corner braking. 92268ARMassertions+26Pythontests; Keil0errors/warnings, B24notphysicalvalidated. BackupB23; export logs beforeflash. No geometryadvance/minspin/forcedpivot onvalid or invented direction onFF.

B25 current: owner reports B24 hardware regression and explicitly requests fixed high Base without straight boost. Read docs/fixed-base-b25.md. Supersedes B15-B24 boost/curve-risk target scheduling and B24 shape floor/common-ramped pivot: TRACK target=Base; ignore legacy straight_percent, menu5 rows with compatibility field synced toBase. Keep12/0/0.12, Base55/Max85; startup ramp remains. Confirmed pending loss entersTURN immediately unless a new opposite split needs its6ms confirmation; direct symmetric min(Max,36) request, per-wheel motor reversal guard unchanged. Weak central +/-0.5 must not overwrite pending/ALIGN/post-turn direction; strong>=1.2 or confirmed split can retarget. Preserve ALIGN30/postturn35, GAP/ambiguity bounds, deadlines, calibration/watchdogs/noautoStart. BaselineB24 repro90048, B25 97520ARM+26Python tests, Keil0errors/warnings; nohardwarevalidation. No geometryADVANCE/minspin. BackupB24; update error log for new issues.
