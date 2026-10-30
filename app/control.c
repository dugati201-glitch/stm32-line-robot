#include "control.h"
#include "control_tuning.h"
static float Abs(float x) { return x < 0.0f ? -x : x; }
static float Limit(float x, float low, float high) { return x < low ? low : x > high ? high : x; }
static float Approach(float current, float target, float step)
{
    if (current < target) return current + step > target ? target : current + step;
    return current - step < target ? target : current - step;
}
void Control_Defaults(ControlParameters *p)
{
    /* Trial gains: error is in sensor pitches, derivative is per second. */
    p->kp=12.00f; p->ki=0.0f; p->kd=0.90f;
    p->base_percent=55; p->straight_percent=55; p->max_percent=85;
    p->advance_ms=0; p->align_ms=CONTROL_REACQUIRE_DEFAULT_MS;
}
void Control_Stop(ControlState *s, ControlFault fault)
{
    s->mode=fault==FAULT_NONE?CONTROL_STOPPED:CONTROL_FAULT; s->fault=fault;
    s->speed=s->integral=s->derivative=s->last_error=s->last_correction=s->saturation_error=0;
    s->target=s->planner_rate=0; s->speed_reason=SPEED_IDLE;
    s->speed_bonus=0; s->cruise_ms=s->speed_guard_ms=0;
    s->curve_valid=0; s->curve_age_ms=1000U; s->curve_left=s->curve_right=0;
    s->stable_ms=s->lost_ms=s->recovery_ms=s->reacquire_ms=s->ambiguous_ms=0;
    s->split_cue_ms=0; s->split_cue_dir=0;
    s->corner_hint_ms=s->saturation_ms=0; s->corner_dir=s->turn_dir=0;
    s->brake_ms=0; s->brake_count=0;
    s->guide_age_ms=CONTROL_GUIDE_MAX_AGE_MS+1U;
    s->guide_left=s->guide_right=0; s->guide_valid=0;
    s->recovery_seen_line=0;
    s->last_side=0; s->last_side_ms=0;
    s->post_turn_ms=0; s->post_turn_dir=0;
    s->shape_ms=s->corner_pending_ms=0; s->shape_dir=s->corner_pending_dir=0;
    s->left=s->right=0; s->history=s->lost=s->may_cross_gap=0;
    s->probe_continue=0;
    s->recovery_arc_speed=0; s->recovery_arc=0;
    s->gap_correction=0;
    s->corner_gain=0; s->search_near_line=0;
    s->fast_center_ms=0; s->fast_exit=0;
    s->d_output=0;
    s->recovery_visible_ms=s->post_turn_visible_ms=0;
    s->drive_corner_dir=0;
    s->line_stop_reason=LINE_STOP_NONE;
}
void Control_Start(ControlState *s) { Control_Stop(s,FAULT_NONE); s->mode=CONTROL_TRACK; }
static void LineFault(ControlState *s, LineStopReason reason)
{
    Control_Stop(s,FAULT_LINE); s->line_stop_reason=reason;
}
static void RememberSide(ControlState *s, int8_t side)
{
    s->last_side=side; s->last_side_ms=CONTROL_LAST_SIDE_MS;
}
static void TurnOutput(ControlState *s, float maximum)
{
    float outer=Limit(s->search_near_line?CONTROL_PIVOT_OUTER_PERCENT:CONTROL_SEARCH_OUTER_PERCENT,0,maximum);
    float inner=Limit(s->search_near_line?CONTROL_PIVOT_INNER_REVERSE_PERCENT:CONTROL_SEARCH_INNER_REVERSE_PERCENT,0,maximum);
    /* B43: forward-biased swing; no timed forward arc or brake dwell. */
    s->recovery_arc=0;
    s->target=outer; s->speed_reason=SPEED_RECOVERY;
    s->left=(int16_t)((s->turn_dir>0?outer:-inner)*10.0f);
    s->right=(int16_t)((s->turn_dir>0?-inner:outer)*10.0f);
    /* Carry only an already earned bounded duty into visible-line handoff.
     * This is neither mean wheel PWM nor a measured vehicle speed. */
    s->speed=Limit(s->speed,0,Limit(CONTROL_TURN_REJOIN_CARRY_PERCENT,0,maximum));
}
static void BeginRecovery(ControlState *s, int8_t direction)
{
    /* Same direct recovery for a first corner and a chained retry. Keep these
     * state fields reserved/zero; never re-enable a timed forward arc. */
    s->recovery_arc_speed=0;
    s->recovery_arc=0;
    /* B24: operator requested direct steering, without a brake-before-turn pulse. */
    s->fast_center_ms=0; s->fast_exit=0;
    s->mode=CONTROL_TURN; s->turn_dir=direction;
    s->recovery_visible_ms=s->post_turn_visible_ms=0;
    s->drive_corner_dir=0;
    s->speed_bonus=0; s->cruise_ms=0; s->speed_guard_ms=CONTROL_SPEED_GUARD_MS;
    /* A loss just after reacquisition belongs to the same bounded episode. */
    if(!s->post_turn_ms) s->recovery_ms=0;
    s->reacquire_ms=0;
    s->post_turn_ms=0; s->post_turn_dir=0;
    s->guide_valid=0;
    s->probe_continue=0;
    s->curve_valid=0;
    s->corner_pending_ms=0; s->corner_pending_dir=0;
    s->recovery_seen_line=0; s->split_cue_ms=0; s->split_cue_dir=0;
    s->integral=s->derivative=s->planner_rate=0; s->lost=1; s->saturation_ms=0;
    s->brake_ms=0;
    s->corner_gain=0;
    s->d_output=0;
}
static void RecoveryOutput(ControlState *s, float maximum)
{
    /* BRAKE enum is retained for decoding historical logs, never emitted now. */
    s->brake_ms=0; s->mode=CONTROL_TURN;
    TurnOutput(s,maximum);
}
static void SearchLastSideOrStop(ControlState *s, float maximum, LineStopReason reason)
{
    if(s->last_side_ms && s->last_side) {
        s->may_cross_gap=0;
        BeginRecovery(s,s->last_side);
        TurnOutput(s,maximum);
    } else LineFault(s,reason);
}
static void ForwardHold(ControlState *s, float base, float maximum, float dt)
{
    float correction;
    s->target=Limit(base,0,s->speed);
    s->speed=Approach(s->speed,s->target,CONTROL_DECEL_PERCENT_PER_SECOND*dt);
    s->last_correction*=0.85f;
    correction=Limit(s->last_correction,-s->speed,s->speed);
    s->left=(int16_t)(Limit(s->speed+correction,0,maximum)*10.0f);
    s->right=(int16_t)(Limit(s->speed-correction,0,maximum)*10.0f);
}
static void AmbiguousHold(ControlState *s, float base, float maximum, float dt)
{
    float average=(s->guide_left+s->guide_right)/20.0f, scale;
    /* Keep only the last trusted wheel ratio, never the invalid centroid.
     * Shed high straight duty toward the existing tight-curve cap; no boost. */
    s->speed=Limit(s->speed,0,average);
    s->target=Limit(base,0,CONTROL_AMBIGUOUS_CAP_PERCENT);
    s->target=Limit(s->target,0,s->speed);
    s->speed=Approach(s->speed,s->target,CONTROL_DECEL_PERCENT_PER_SECOND*dt);
    scale=average>0?s->speed/average:0;
    s->mode=CONTROL_PROBE; s->speed_reason=SPEED_PROBE;
    s->left=(int16_t)Limit(s->guide_left*scale,0,maximum*10.0f);
    s->right=(int16_t)Limit(s->guide_right*scale,0,maximum*10.0f);
    s->last_correction=(s->left-s->right)/20.0f;
}
static void UpdateDerivative(ControlState *s, float e, float dt)
{
    s->derivative+=(dt/(CONTROL_D_FILTER_SECONDS+dt))*((e-s->last_error)/dt-s->derivative);
    /* Planner rate is separate from the PD derivative. */
    s->planner_rate+=(dt/(CONTROL_SPEED_RATE_FILTER_SECONDS+dt))*(s->derivative-s->planner_rate);
    s->last_error=e;
}
static void RememberGuide(ControlState *s, float e, uint8_t mask, uint32_t dt_ms, float maximum)
{
    /* A genuine edge/outward drift invalidates straight-gap continuation immediately. */
    if(Abs(e)>1.2f || (mask&0xC3U) || Abs(s->planner_rate)>CONTROL_GUIDE_RATE_MAX) {
        s->guide_valid=0; s->stable_ms=0;
        return;
    }
    if(Abs(e)<=CONTROL_GUIDE_CENTER_PITCH && s->left>0 && s->right>0) {
        float correction=(s->left-s->right)/20.0f;
        float previous=s->guide_valid && s->guide_age_ms<=4U?s->gap_correction:0;
        /* Filter only the stored gap direction, not live TRACK PID. A single
         * central-eye D kick must not be held for a whole white dash. */
        if(Abs(correction-previous)>=0.05f) {
            float mean=(s->left+s->right)/20.0f;
            float bound=Limit(mean,0,maximum-mean);
            correction=previous+CONTROL_GAP_STEER_ALPHA_PER_MS*dt_ms*(correction-previous);
            correction=Limit(correction,-bound,bound);
        }
        if(Abs(correction)<0.05f) correction=0; /* Below half one PWM permille. */
        s->gap_correction=correction;
        s->guide_left=s->left; s->guide_right=s->right;
        s->guide_age_ms=0; s->guide_valid=1;
        if(s->stable_ms<1000U) s->stable_ms+=dt_ms;
        if(s->stable_ms>=40U) { s->corner_dir=0; s->corner_hint_ms=0; }
        if(s->stable_ms>=CONTROL_LAST_SIDE_CLEAR_MS) { s->last_side=0; s->last_side_ms=0; }
        if(s->stable_ms>=CONTROL_CORNER_CLEAR_MS) {
            s->corner_pending_ms=0; s->corner_pending_dir=0;
            s->drive_corner_dir=0;
        }
        if(s->stable_ms>=CONTROL_POST_TURN_CLEAR_MS && s->post_turn_ms) {
            s->post_turn_ms=0; s->post_turn_dir=0; s->recovery_ms=0;
        }
    } else s->stable_ms=0;
}
static void HoldGuide(ControlState *s, float base, float maximum)
{
    float average=(s->guide_left+s->guide_right)/20.0f, scale=1.0f;
    /* Short gaps keep exact commands. Longer gaps shed only the excess above
     * Base, preserving the remembered wheel ratio and never accelerating. */
    if(average>base && s->lost_ms>CONTROL_GAP_BONUS_HOLD_MS) {
        float drop=(s->lost_ms-CONTROL_GAP_BONUS_HOLD_MS)*0.001f*CONTROL_GAP_BONUS_DECEL;
        scale=(average-Limit(drop,0,average-base))/average;
    }
    s->mode=CONTROL_GAP; s->speed_reason=SPEED_GAP;
    s->left=(int16_t)Limit((average+s->gap_correction)*10.0f*scale,0,maximum*10.0f);
    s->right=(int16_t)Limit((average-s->gap_correction)*10.0f*scale,0,maximum*10.0f);
    s->speed=(s->left+s->right)/20.0f; s->target=s->speed;
    s->last_correction=(s->left-s->right)/20.0f;
}
static void Follow(ControlState *s, const ControlParameters *p, float e, float target, float maximum, float dt, uint8_t smooth, uint8_t mask, uint8_t outward)
{
    float steering_limit, steering_low, steering_high, candidate, correction, common, candidate_common, proportional_gain;
    float tight=0, reverse_limit=0, branch_bias=0;
    float common_reduction=CONTROL_TIGHT_COMMON_REDUCTION;
    int8_t reserve_dir=0;
    uint8_t outer_grip;
    float accel=CONTROL_ACCEL_PERCENT_PER_SECOND;
    float decel;
    /* B46: reduce the current target immediately with offset, without a timer.
     * Keep low Base settings below the floor; ALIGN retains its B45 handoff. */
    if(s->mode==CONTROL_TRACK && Abs(e)>CONTROL_CURVE_SLOW_START_PITCH &&
       target>CONTROL_CURVE_FLOOR_PERCENT) {
        float curve_floor=Limit(CONTROL_CURVE_FLOOR_PERCENT,0,target);
        target=Limit(target-(Abs(e)-CONTROL_CURVE_SLOW_START_PITCH)*
                     CONTROL_CURVE_SLOW_PER_PITCH,curve_floor,target);
    }
    decel=smooth && target>=p->base_percent?
        CONTROL_SOFT_DECEL_PERCENT_PER_SECOND:CONTROL_DECEL_PERCENT_PER_SECOND;
    if(s->mode==CONTROL_TRACK && Abs(e)>CONTROL_CURVE_SLOW_START_PITCH && target<s->speed)
        decel=CONTROL_DECEL_PERCENT_PER_SECOND;
    if(s->post_turn_ms && !s->corner_pending_ms && Abs(e)<=1.0f &&
       s->speed<CONTROL_EXIT_ACCEL_CEILING_PERCENT) accel=CONTROL_EXIT_ACCEL_PERCENT_PER_SECOND;
    /* Only calm narrow-line data after an earned fast handoff can use the
     * faster exit ramp. Raw high D blocks it even if applied D is limited. */
    if(s->post_turn_ms && s->fast_exit && !s->corner_pending_ms &&
       Abs(e)<=CONTROL_FAST_EXIT_ACCEL_PITCH && (mask&0x18U) && !(mask&0xE7U) &&
       Abs(p->kd*s->derivative)<=CONTROL_FAST_EXIT_RAW_D_MAX)
        accel=CONTROL_FAST_EXIT_ACCEL_PERCENT_PER_SECOND;
    s->target=target;
    s->speed=Approach(s->speed,target,(target>s->speed?accel:decel)*dt);
    steering_limit=Limit(s->speed,0,maximum*0.5f);
    /* B45: raw step and filtered rate must both still point outward.
     * Outer eyes may grant authority without a prior broad-shape latch. */
    outer_grip=outward && Abs(e)>=CONTROL_TIGHT_FULL_PITCH && e*s->derivative>=0 &&
        ((e>0 && (mask&0xC0U)) || (e<0 && (mask&0x03U)));
    if(s->mode==CONTROL_TRACK && outward && e*s->derivative>=0 &&
       (outer_grip || (s->corner_pending_ms && s->drive_corner_dir &&
        s->drive_corner_dir==s->corner_pending_dir &&
        e*s->corner_pending_dir>CONTROL_TIGHT_START_PITCH &&
        ((s->corner_pending_dir>0?(mask&0xC0U):(mask&0x03U)) ||
         s->shape_ms>=CONTROL_SHAPE_CONFIRM_MS)))) {
        tight=Limit((Limit(s->speed,0,target)-CONTROL_TIGHT_DUTY_START_PERCENT)/
                    (CONTROL_TIGHT_DUTY_FULL_PERCENT-CONTROL_TIGHT_DUTY_START_PERCENT),0,1)*
              Limit((Abs(e)-CONTROL_TIGHT_START_PITCH)/
                    (CONTROL_TIGHT_FULL_PITCH-CONTROL_TIGHT_START_PITCH),0,1);
        reverse_limit=Limit(CONTROL_TIGHT_REVERSE_PERCENT,0,maximum*0.5f)*tight;
        reserve_dir=e>0?1:-1;
        if(outer_grip) {
            /* A visible outer cue commits the next loss to this side now. */
            s->corner_pending_dir=reserve_dir;
            s->corner_pending_ms=CONTROL_CORNER_PENDING_MS;
            RememberSide(s,reserve_dir);
        }
        if(outer_grip && !s->shape_ms) common_reduction=CONTROL_OUTER_COMMON_REDUCTION;
    }
    /* B41: ramp branch bias from 2ms of visible directional shape, reaching
     * full bias at 4ms confirmation. This does not latch a new search side. */
    if(s->mode==CONTROL_TRACK && s->shape_ms>=CONTROL_BRANCH_BIAS_START_MS &&
       e*s->shape_dir>=1.0f && outward &&
       s->shape_dir*s->derivative>=-CONTROL_TIGHT_INWARD_RATE_PITCH_PER_SECOND) {
        float duty_weight=Limit((Limit(s->speed,0,target)-CONTROL_TIGHT_DUTY_START_PERCENT)/
            (CONTROL_TIGHT_DUTY_FULL_PERCENT-CONTROL_TIGHT_DUTY_START_PERCENT),0,1);
        branch_bias=s->shape_dir*p->kp*duty_weight*
            Limit((float)s->shape_ms/CONTROL_SHAPE_CONFIRM_MS,0,1)*
            Limit(CONTROL_BRANCH_AIM_PITCH-Abs(e),0,CONTROL_BRANCH_AIM_PITCH-1.0f);
    }
    /* Preserve ordinary counter-steering bounds. Only the observed turn side
     * gets extra authority; full reserve allows up to 24% inner reverse at Max85. */
    steering_low=-steering_limit-(reserve_dir<0?reverse_limit:0);
    steering_high=steering_limit+(reserve_dir>0?reverse_limit:0);
    {
        float demand=p->kd*s->derivative;
        if(p->kd>=CONTROL_CORNER_D_MIN_KD &&
           (s->corner_pending_ms || s->post_turn_ms || s->mode==CONTROL_ALIGN ||
            (Abs(e)<=CONTROL_FAST_EXIT_ACCEL_PITCH && (mask&0x18U) && !(mask&0xE7U)))) {
            float cap=Limit(CONTROL_CORNER_D_MAX_PERCENT,0,steering_limit);
            float previous=Limit(s->d_output,-cap,cap);
            demand=Limit(demand,-cap,cap);
            /* Discard old-sign D before building the new sign. Falling
             * demand releases now, so the limiter cannot prolong old yaw. */
            if(demand*previous<=0) previous=0;
            s->d_output=Abs(demand)<=Abs(previous)?demand:
                Approach(previous,demand,CONTROL_CORNER_D_RISE_PER_SECOND*dt);
        } else s->d_output=demand;
    }
    /* Start steering harder on the first directional visible shape, without B24's
     * abrupt correction floor. A cue is heuristic, not proof of a 90deg turn.
     * Ordinary soft curves, improving error and RECENTER keep ordinary PD. */
    if((s->mode==CONTROL_TRACK && outward && e*s->derivative>=0 && s->shape_ms &&
        e*s->shape_dir>=1.0f && e*s->planner_rate>=0) || tight>0) {
        float desired=tight*CONTROL_CORNER_GAIN_MAX;
        if(s->shape_ms && e*s->shape_dir>=1.0f && e*s->planner_rate>=0) {
            float broad=CONTROL_CORNER_GAIN_MAX*Limit(Abs(e)-1.0f,0,1);
            if(broad>desired) desired=broad;
        }
        s->corner_gain=Approach(s->corner_gain,desired,CONTROL_CORNER_GAIN_RISE_PER_MS*dt*1000.0f);
    } else s->corner_gain=0;
    proportional_gain=p->kp*(1.0f+CONTROL_POSITION_KP_BOOST*Limit(Abs(e)/3.5f,0,1));
    if(s->corner_gain>0) proportional_gain*=1.0f+s->corner_gain;
    candidate=p->ki>0?Limit(s->integral+e*dt,-10.0f/p->ki,10.0f/p->ki):0;
    correction=proportional_gain*e+p->ki*candidate+s->d_output+branch_bias;
    candidate_common=Limit(s->speed*(1.0f-common_reduction*tight),0,maximum-Abs(correction));
    /* Expanded correction bounds may still clip the inner wheel after the
     * common reduction. Do not integrate into that actuator saturation. */
    if((correction>=steering_low && correction<=steering_high &&
        candidate_common+correction>=-reverse_limit && candidate_common+correction<=maximum &&
        candidate_common-correction>=-reverse_limit && candidate_common-correction<=maximum) ||
       correction*e<0) s->integral=candidate;
    correction=Limit(proportional_gain*e+p->ki*s->integral+s->d_output+branch_bias,steering_low,steering_high);
    /* B26: visible line gets its P correction in this frame. D already has a
     * 10 ms low-pass; a second slew on the whole PD delayed ordinary bends. */
    s->last_correction=correction;
    common=Limit(s->speed*(1.0f-common_reduction*tight),0,maximum-Abs(correction));
    s->left=(int16_t)(Limit(common+correction,-reverse_limit,maximum)*10.0f);
    s->right=(int16_t)(Limit(common-correction,-reverse_limit,maximum)*10.0f);
}
/* A split pattern may still supply direction, never a trustworthy centroid.
 * Require three outer-side eyes and at most one opposite-side eye. */
static int8_t SplitDirection(uint8_t mask)
{
    unsigned i, left=0, right=0;
    for(i=0;i<4;++i) { left+=(mask>>i)&1U; right+=(mask>>(i+4U))&1U; }
    if((mask&0x80U) && right>=3 && left<=1) return 1;
    if((mask&0x01U) && left>=3 && right<=1) return -1;
    return 0;
}
static int8_t ConfirmSplit(ControlState *s, uint8_t mask, uint32_t dt_ms)
{
    int8_t cue=SplitDirection(mask);
    if(cue && cue==s->split_cue_dir) {
        if(s->split_cue_ms<CONTROL_LOSS_CONFIRM_MS) s->split_cue_ms+=dt_ms;
    } else { s->split_cue_dir=cue; s->split_cue_ms=cue?dt_ms:0; }
    return s->split_cue_ms>=CONTROL_LOSS_CONFIRM_MS?cue:0;
}
static void ObserveValidShape(ControlState *s, uint8_t valid, uint8_t mask, uint32_t dt_ms)
{
    /* Edge-reaching directional group anchored in central eyes. Four or
     * more consecutive eyes reaching an edge (F0/0F) qualify via SplitDirection;
     * confirm in 4ms (two nominal frames), never from width alone or mask FF. */
    int8_t side=(valid && (mask&0x18U))?SplitDirection(mask):0;
    if(side && side==s->shape_dir) {
        if(s->shape_ms<CONTROL_SHAPE_CONFIRM_MS) s->shape_ms+=dt_ms;
    } else {
        /* A narrow same-corner edge may still use its confirmed pending cue.
         * Follow clears assistance whenever neither visible policy qualifies. */
        if(side) s->corner_gain=0;
        s->shape_dir=side; s->shape_ms=side?dt_ms:0;
    }
    if(side && s->shape_ms>=CONTROL_SHAPE_CONFIRM_MS) {
        s->corner_pending_dir=side; s->corner_pending_ms=CONTROL_CORNER_PENDING_MS;
        s->drive_corner_dir=side;
        RememberSide(s,side);
    }
}
/* Valid connected group, at most three eyes, away from the two extremes. */
static uint8_t NarrowTravel(uint8_t mask)
{
    uint8_t count=0, bits=mask;
    if(!mask || (mask&0x81U)) return 0;
    while(bits) { count+=(bits&1U); bits>>=1; }
    return count<=3U;
}
void Control_Step(ControlState *s, const ControlParameters *p, int16_t error, uint8_t valid, uint8_t mask, uint32_t dt_ms)
{
    float e=error/1000.0f, dt=dt_ms*0.001f, base, maximum;
    float step=e-s->last_error, observation_dt=dt;
    /* Ignore tiny centroid jitter when deciding reserve eligibility. A clear
     * returning step still releases now; live P/D never gets this deadband. */
    uint8_t recovering, outward=valid && (e>=0?step:-step)>=-CONTROL_TIGHT_RETURN_STEP_PITCH;
    int8_t split_direction;
    if(s->mode==CONTROL_STOPPED || s->mode==CONTROL_FAULT) return;
    if(dt_ms<1U || dt_ms>4U) { Control_Stop(s,FAULT_TIMING); return; }
    maximum=Limit((float)p->max_percent,0,100);
    base=Limit((float)p->base_percent,0,maximum);
    if(base==0 || maximum==0) { Control_Stop(s,FAULT_NONE); return; }
    s->search_near_line=mask!=0U;
    if(s->curve_age_ms<1000U) s->curve_age_ms+=dt_ms;
    if(s->speed_guard_ms>dt_ms) s->speed_guard_ms-=dt_ms;
    else s->speed_guard_ms=0;
    if(s->guide_age_ms<1000U) s->guide_age_ms+=dt_ms;
    if(s->corner_pending_ms>dt_ms) s->corner_pending_ms-=dt_ms;
    else { s->corner_pending_ms=0; s->corner_pending_dir=0; s->drive_corner_dir=0; }
    recovering=s->mode==CONTROL_TURN || s->mode==CONTROL_SEARCH || s->mode==CONTROL_ALIGN || s->mode==CONTROL_BRAKE;
    /* Charge elapsed time once, even if a legacy state has both guards set.
     * Saturate the budget while visible data is authoritative. */
    if(recovering || s->post_turn_ms) {
        if(s->recovery_ms<CONTROL_RECOVERY_MAX_MS) {
            s->recovery_ms+=dt_ms;
            if(s->recovery_ms>CONTROL_RECOVERY_MAX_MS) s->recovery_ms=CONTROL_RECOVERY_MAX_MS;
        }
    }
    if(s->post_turn_ms) {
        if(s->post_turn_ms>dt_ms) s->post_turn_ms-=dt_ms;
        else {
            s->post_turn_ms=0; s->post_turn_dir=0;
            if(!recovering) s->recovery_ms=0;
        }
    }
    if(s->last_side_ms>dt_ms) s->last_side_ms-=dt_ms;
    else { s->last_side_ms=0; s->last_side=0; }
    if(s->corner_hint_ms>dt_ms) s->corner_hint_ms-=dt_ms;
    else { s->corner_hint_ms=0; s->corner_dir=0; }
    /* A newly valid frame gets PD first. A glimpse does not erase the budget:
     * the next invalid frame still faults unless sustained tracking earned it. */
    if(!valid && (recovering || s->post_turn_ms) && s->recovery_ms>=CONTROL_RECOVERY_MAX_MS) {
        LineFault(s,LINE_STOP_TURN_TIMEOUT); return;
    }
    if(valid || mask==0U) { s->split_cue_ms=0; s->split_cue_dir=0; }
    ObserveValidShape(s,valid,mask,dt_ms);
    /* Distinct narrow travel must precede a confirmed new corner of either sign.
     * Latch the earned segment through shape confirmation, never through loss. */
    if(!recovering && s->post_turn_ms && valid) {
        if(NarrowTravel(mask)) {
            if(s->post_turn_visible_ms<CONTROL_NEW_CORNER_TRACK_MS)
                s->post_turn_visible_ms+=(uint16_t)dt_ms;
        } else if(s->post_turn_visible_ms<CONTROL_NEW_CORNER_TRACK_MS)
            s->post_turn_visible_ms=0;
        if(s->post_turn_visible_ms>=CONTROL_NEW_CORNER_TRACK_MS &&
           s->shape_ms>=CONTROL_SHAPE_CONFIRM_MS && e*s->shape_dir>=1.0f) {
            s->post_turn_ms=0; s->post_turn_dir=0; s->post_turn_visible_ms=0;
            s->recovery_ms=0;
        }
    } else s->post_turn_visible_ms=0;
    /* Continuous inner-line evidence can complete a moderate curve even if
     * error never reaches the narrow-center handoff. Keep D and side memory. */
    if((recovering || s->post_turn_ms) && valid && Abs(e)<=2.0f &&
       (mask&0x3CU) && !(mask&0x81U)) {
        if(s->recovery_visible_ms<CONTROL_RECOVERY_VISIBLE_MS)
            s->recovery_visible_ms+=(uint16_t)dt_ms;
        if(s->recovery_visible_ms>=CONTROL_RECOVERY_VISIBLE_MS) s->recovery_ms=0;
    } else s->recovery_visible_ms=0;
    /* Only consecutive narrow central data earns fast handoff. Continue
     * observing in post-turn TRACK if a shorter legacy align_ms completed first. */
    if((recovering || s->post_turn_ms) && valid && Abs(e)<=CONTROL_FAST_CENTER_PITCH &&
       (mask&0x18U) && !(mask&0xE7U) && !s->corner_pending_ms) {
        if(s->fast_center_ms<CONTROL_FAST_CENTER_MS) s->fast_center_ms+=(uint16_t)dt_ms;
    } else s->fast_center_ms=0;
    if(s->fast_center_ms>=CONTROL_FAST_CENTER_MS) s->fast_exit=1;
    if(!valid || Abs(e)>1.5f || s->shape_ms>=CONTROL_SHAPE_CONFIRM_MS || (!recovering && !s->post_turn_ms)) {
        s->fast_exit=0;
    }
    if(recovering || s->shape_ms>=CONTROL_SHAPE_CONFIRM_MS || (valid && Abs(e)>=2.4f))
        s->speed_guard_ms=CONTROL_SPEED_GUARD_MS;
    if(valid) {
        /* Keep observed side separate from PID error, which can pass through zero. */
        if(Abs(e)>=1.2f) {
            RememberSide(s,e>0?1:-1);
            if(s->corner_pending_ms && !s->shape_dir) s->corner_pending_dir=e>0?1:-1;
        }
        /* A valid extreme error is not a command to run a timed corner routine. */
        if(recovering) {
            uint8_t visible_settled=s->recovery_visible_ms>=CONTROL_RECOVERY_VISIBLE_MS;
            float rejoin=(s->fast_exit || visible_settled)?base:Limit(base,0,CONTROL_RECENTER_PERCENT);
            uint8_t centered,settled;
            s->recovery_seen_line=1;
            /* Retain a genuinely new side even if a weak center sample follows
             * it before the next loss. Weak readings must not undo this cue. */
            if(Abs(e)>=1.2f) s->turn_dir=e>0?1:-1;
            if(s->mode!=CONTROL_ALIGN) {
                s->last_error=e; s->derivative=s->integral=s->planner_rate=0;
                /* Do not discard the current command magnitude by resetting
                 * every reacquisition to 30%. Keep a bounded forward handoff;
                 * a low-duty approach must not jump up to the rejoin ceiling. */
                s->speed=Limit(s->speed,0,rejoin);
            }
            s->brake_ms=0; /* Visible line immediately cancels a brake pulse as well as a swing turn. */
            s->mode=CONTROL_ALIGN; /* RECENTER: immediately follow the observed line with forward PD. */
            if(Abs(e)<=1.0f && (mask&0x3CU)) s->reacquire_ms+=dt_ms;
            else s->reacquire_ms=0;
            /* B36: only a narrow central single line can earn the fast exit.
             * A glimpse/broad patch cannot. Keep memory and episode budget. */
            centered=s->reacquire_ms>=(uint32_t)Limit(p->align_ms,CONTROL_REACQUIRE_MIN_MS,CONTROL_REACQUIRE_MAX_MS);
            settled=centered || s->fast_exit || visible_settled;
            /* B46: preserve damping through centered ALIGN completion. */
            UpdateDerivative(s,e,dt);
            s->speed_reason=SPEED_RECOVERY;
            Follow(s,p,e,rejoin,maximum,dt,0,mask,outward);
            if(settled) {
                s->mode=CONTROL_TRACK; s->lost=0; s->lost_ms=0;
                s->post_turn_ms=CONTROL_POST_TURN_MS;
                s->post_turn_dir=s->turn_dir;
                s->saturation_ms=0;
                s->corner_hint_ms=0; s->corner_dir=0; s->stable_ms=0;
                s->ambiguous_ms=0; s->may_cross_gap=0;
                /* Publish current forward commands at the handoff, not one frame later. */
                RememberGuide(s,e,mask,dt_ms,maximum);
            }
            return;
        }
        /* Fixed Base: there is no straight bonus to restore after a dash. */
        s->speed_bonus=0; s->cruise_ms=0;
        /* Preserve the observed slope across a short nonwhite PROBE dropout.
         * White GAP/recovery/long loss still reset stale derivative history. */
        if(!s->history || (s->lost && !(s->mode==CONTROL_PROBE &&
           s->ambiguous_ms && s->shape_ms && s->lost_ms<=CONTROL_BRANCH_PROBE_KEEP_MS))) {
            s->last_error=e; s->derivative=s->integral=s->planner_rate=0;
        } else if(s->lost) observation_dt+=(float)s->lost_ms*0.001f;
        s->lost=0; s->lost_ms=s->ambiguous_ms=0; s->history=1; s->mode=CONTROL_TRACK;
        s->probe_continue=0;
        if(Abs(e)>=1.2f && (mask&0x81U)) {
            s->corner_dir=e>0?1:-1; s->corner_hint_ms=CONTROL_CORNER_MEMORY_MS;
        }
        UpdateDerivative(s,e,observation_dt);
        {
            float target=base;
            s->speed_reason=SPEED_BASE;
            if(s->shape_ms>=CONTROL_SHAPE_CONFIRM_MS) {
                /* Follow applies B46 offset slowing to the current Base target. */
                s->speed_reason=SPEED_CORNER;
            }
            if(s->post_turn_ms && !s->fast_exit && s->recovery_visible_ms<CONTROL_RECOVERY_VISIBLE_MS &&
               !s->recovery_arc && s->stable_ms<CONTROL_REJOIN_CENTER_MS) {
                target=Limit(target,0,CONTROL_REJOIN_CAP_PERCENT);
                s->speed_reason=SPEED_RECOVERY;
            }
            Follow(s,p,e,target,maximum,dt,
                   !s->corner_pending_ms && !s->post_turn_ms && !(mask&0x81U) &&
                   Abs(e)<=1.5f && Abs(s->planner_rate)<=4.0f,mask,outward);
        }
        RememberGuide(s,e,mask,dt_ms,maximum);
        /* Separate from the long straight-gap guide: this snapshot tolerates
         * only a few milliseconds of missing/fragmented moderate curve data. */
        s->curve_valid=!s->corner_pending_ms && !s->post_turn_ms && !s->corner_hint_ms &&
            !(mask&0x81U) && Abs(e)<=2.0f && Abs(s->planner_rate)<=4.0f && s->left>0 && s->right>0;
        if(s->curve_valid) {
            s->curve_left=s->left; s->curve_right=s->right; s->curve_age_ms=0;
        }
        /* Long saturation with no improving edge error is different from a normal curve sample. */
        if(Abs(e)>=2.4f && (mask&0x81U) && (s->left==0 || s->right==0) && s->speed>=5.0f) {
            if(!s->saturation_ms || Abs(e)<s->saturation_error-0.2f) {
                s->saturation_ms=dt_ms; s->saturation_error=Abs(e);
            } else s->saturation_ms+=dt_ms;
            if(s->saturation_ms>5000U) s->saturation_ms=5000U;
            /* Diagnostic only: a sustained valid curve must not be reclassified as a corner. */
        } else s->saturation_ms=0;
        return;
    }
    if(!s->history) { LineFault(s,LINE_STOP_NO_HISTORY); return; }
    s->integral=0;
    s->corner_gain=0;
    s->cruise_ms=0; s->speed_bonus=0;
    if(!s->lost) {
        /* Latch once per continuous loss. Center flicker must earn a new
         * trusted 40 ms guide before another allowance; corner chains bypass. */
        s->probe_continue=!recovering && !s->post_turn_ms && !s->corner_pending_ms &&
            !s->corner_hint_ms && !s->speed_guard_ms && s->guide_valid &&
            s->guide_age_ms<=CONTROL_AMBIGUOUS_GUIDE_AGE_MS &&
            s->stable_ms>=CONTROL_AMBIGUOUS_CENTER_MS && Abs(s->last_error)<=CONTROL_GUIDE_CENTER_PITCH &&
            Abs(s->planner_rate)<=4.0f && s->guide_left>0 && s->guide_right>0 &&
            (mask&0x18U)==0x18U && !SplitDirection(mask);
    }
    s->stable_ms=0;
    split_direction=ConfirmSplit(s,mask,dt_ms);
    if(recovering) {
        if(s->mode==CONTROL_ALIGN && Abs(s->last_error)>=1.2f) s->turn_dir=s->last_error>0?1:-1;
        /* A new side after visible reacquisition may be the next bend.
         * Retarget within this recovery episode; never restart its deadline. */
        if(s->recovery_seen_line && split_direction) {
            RememberSide(s,split_direction);
            s->turn_dir=split_direction;
            s->corner_dir=split_direction; s->corner_hint_ms=CONTROL_CORNER_MEMORY_MS;
        }
        s->mode=CONTROL_TURN;
        s->reacquire_ms=0;
        RecoveryOutput(s,maximum); return;
    }
    if(!s->lost) {
        s->lost=1; s->lost_ms=0;
        s->may_cross_gap=!s->post_turn_ms && !s->corner_pending_ms && s->guide_valid && s->guide_age_ms<=CONTROL_GUIDE_MAX_AGE_MS;
    }
    s->lost_ms+=dt_ms;
    if((mask&0x18U)!=0x18U || s->lost_ms>CONTROL_AMBIGUOUS_BRIDGE_MS)
        s->probe_continue=0;
    /* ConfirmSplit has already observed this frame. Edge/corner/post-turn data
     * bypass this bridge, and the ordinary loss/deadline counters never reset. */
    if(!s->may_cross_gap && s->curve_valid && !s->corner_pending_ms && !s->post_turn_ms &&
       !s->corner_hint_ms && !split_direction && !(mask&0x81U) &&
       s->curve_age_ms<=CONTROL_CURVE_SNAPSHOT_MAX_AGE_MS &&
       s->lost_ms<=(mask?CONTROL_CURVE_BRIDGE_MS:CONTROL_CURVE_WHITE_BRIDGE_MS)) {
        s->mode=CONTROL_PROBE; s->speed_reason=SPEED_PROBE;
        s->left=(int16_t)Limit(s->curve_left,0,maximum*10.0f);
        s->right=(int16_t)Limit(s->curve_right,0,maximum*10.0f);
        s->speed=(s->left+s->right)/20.0f; s->target=s->speed;
        if(mask) s->ambiguous_ms+=dt_ms;
        else s->ambiguous_ms=0;
        return;
    }
    if(mask!=0U) {
        s->ambiguous_ms+=dt_ms;
        if(split_direction) {
            RememberSide(s,split_direction);
            s->may_cross_gap=0; s->guide_valid=0;
            s->corner_dir=split_direction; s->corner_hint_ms=CONTROL_CORNER_MEMORY_MS;
            BeginRecovery(s,split_direction);
            TurnOutput(s,maximum);
            return;
        }
        /* Opposite split evidence may be the next corner: let its existing
         * 6 ms confirmation finish before committing to the older heading. */
        if(s->corner_pending_ms &&
           (!s->split_cue_dir || s->split_cue_dir==s->corner_pending_dir)) {
            s->may_cross_gap=0;
            BeginRecovery(s,s->corner_pending_dir);
            TurnOutput(s,maximum);
            return;
        }
        /* Brief central split masks at a dash boundary must not collapse duty to PROBE 15%. */
        if(s->may_cross_gap && !(mask&0x81U) &&
           s->ambiguous_ms<=CONTROL_GAP_TRANSITION_MS && s->lost_ms<=CONTROL_GAP_MAX_MS) {
            HoldGuide(s,base,maximum); return;
        }
        s->may_cross_gap=0; s->guide_valid=0;
        if(s->probe_continue) {
            if(s->lost_ms<=CONTROL_AMBIGUOUS_BRIDGE_MS && (mask&0x18U)==0x18U &&
               !s->post_turn_ms && !s->corner_pending_ms && !s->corner_hint_ms) {
                AmbiguousHold(s,base,maximum,dt); return;
            }
            s->probe_continue=0;
        }
        if(s->ambiguous_ms>=CONTROL_AMBIGUOUS_MAX_MS) {
            if(s->last_side_ms && s->last_side) {
                SearchLastSideOrStop(s,maximum,LINE_STOP_AMBIGUOUS);
            } else if(s->corner_hint_ms && s->corner_dir) {
                BeginRecovery(s,s->corner_dir);
                TurnOutput(s,maximum);
            } else SearchLastSideOrStop(s,maximum,LINE_STOP_AMBIGUOUS);
        } else { s->mode=CONTROL_PROBE; s->speed_reason=SPEED_PROBE; ForwardHold(s,Limit(base,0,15.0f),maximum,dt); }
        return;
    }
    s->probe_continue=0; /* White loss never borrows the ambiguous-pattern bridge. */
    s->ambiguous_ms=0;
    if(s->may_cross_gap) {
        if(s->lost_ms<=CONTROL_GAP_MAX_MS) HoldGuide(s,base,maximum);
        else SearchLastSideOrStop(s,maximum,LINE_STOP_GAP_TIMEOUT);
        return;
    }
    /* A corner already confirmed on visible line needs no second loss delay.
     * A fresh strong opposite observation updates pending_dir above. */
    if(s->corner_pending_ms) {
        s->may_cross_gap=0;
        BeginRecovery(s,s->corner_pending_dir);
        TurnOutput(s,maximum);
        return;
    }
    if(s->lost_ms<CONTROL_LOSS_CONFIRM_MS) {
        s->mode=CONTROL_PROBE; s->speed_reason=SPEED_PROBE; ForwardHold(s,base,maximum,dt); return;
    }
    {
        int8_t direction=0;
        /* A central eye at +/-0.5 pitch must not reverse a remembered turn.
         * Strong new visible side or a confirmed split can still retarget. */
        if(s->post_turn_ms) {
            direction=s->last_side_ms?s->last_side:s->post_turn_dir;
        } else if(Abs(s->last_error)>=1.2f) direction=s->last_error>0?1:-1;
        else if(s->last_side_ms) direction=s->last_side;
        else if(s->corner_hint_ms) direction=s->corner_dir;
        else if(Abs(s->last_error)>0.35f) direction=s->last_error>0?1:-1;
        if(!direction) { SearchLastSideOrStop(s,maximum,LINE_STOP_GAP_TIMEOUT); return; }
        BeginRecovery(s,direction);
        TurnOutput(s,maximum);
    }
}
