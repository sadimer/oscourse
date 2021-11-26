/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <kern/kclock.h>
#include <kern/timer.h>
#include <inc/time.h>
#include <kern/trap.h>
#include <kern/picirq.h>

/* HINT: Note that selected CMOS
 * register is reset to the first one
 * after first access, i.e. it needs to be selected
 * on every access.
 *
 * Don't forget to disable NMI for the time of
 * operation (look up for the appropriate constant in kern/kclock.h)
 *
 * Why it is necessary?
 */

uint8_t
cmos_read8(uint8_t reg) {
    /* MC146818A controller */
    // LAB 4: Your code here
    nmi_disable();
	outb(CMOS_CMD, reg);
    uint8_t res = inb(CMOS_DATA);
    nmi_enable();
    return res;
}

void
cmos_write8(uint8_t reg, uint8_t value) {
    // LAB 4: Your code here
    nmi_disable();
    outb(CMOS_CMD, reg);
	outb(CMOS_DATA, value);
    nmi_enable();
}

uint16_t
cmos_read16(uint8_t reg) {
    return cmos_read8(reg) | (cmos_read8(reg + 1) << 8);
}

static void
rtc_timer_pic_interrupt(void) {
    // LAB 4: Your code here
    // Enable PIC interrupts.
    pic_irq_unmask(IRQ_CLOCK);
}

static void
rtc_timer_pic_handle(void) {
    rtc_check_status();
    pic_send_eoi(IRQ_CLOCK);
}

struct Timer timer_rtc = {
        .timer_name = "rtc",
        .timer_init = rtc_timer_init,
        .enable_interrupts = rtc_timer_pic_interrupt,
        .handle_interrupts = rtc_timer_pic_handle,
};

static int
get_time(void) {
    struct tm time;

    uint8_t s, m, h, d, M, y, Y, state;
    s = cmos_read8(RTC_SEC);
    m = cmos_read8(RTC_MIN);
    h = cmos_read8(RTC_HOUR);
    d = cmos_read8(RTC_DAY);
    M = cmos_read8(RTC_MON);
    y = cmos_read8(RTC_YEAR);
    Y = cmos_read8(RTC_YEAR_HIGH);
    state = cmos_read8(RTC_BREG);

    if (state & RTC_12H) {
        /* Fixup 12 hour mode */
        h = (h & 0x7F) + 12 * !!(h & 0x80);
    }

    if (!(state & RTC_BINARY)) {
        /* Fixup binary mode */
        s = BCD2BIN(s);
        m = BCD2BIN(m);
        h = BCD2BIN(h);
        d = BCD2BIN(d);
        M = BCD2BIN(M);
        y = BCD2BIN(y);
        Y = BCD2BIN(Y);
    }

    time.tm_sec = s;
    time.tm_min = m;
    time.tm_hour = h;
    time.tm_mday = d;
    time.tm_mon = M - 1;
    time.tm_year = y + Y * 100 - 1900;

    return timestamp(&time);
}

int
gettime(void) {
    // LAB 12: your code here
	nmi_disable();
	while (cmos_read8(RTC_AREG) & RTC_UPDATE_IN_PROGRESS);
	int res = get_time();
	if (res != get_time()) {
		res = get_time();
	}
	nmi_enable();
    return res;
}

void
rtc_timer_init(void) {
    // LAB 4: Your code here
    // (use cmos_read8/cmos_write8)
	uint8_t reg_b = cmos_read8(RTC_BREG);
	reg_b |= RTC_PIE;
	cmos_write8(RTC_BREG, reg_b);
	// 0.5 sec
	uint8_t reg_a = cmos_read8(RTC_AREG);
	reg_a |= 0x0F;
	cmos_write8(RTC_AREG, reg_a);
}

uint8_t
rtc_check_status(void) {
    // LAB 4: Your code here
    // (use cmos_read8)
    return cmos_read8(RTC_CREG);
}
