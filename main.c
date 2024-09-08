#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/termios.h>

#define ADDRSIZE sizeof(intptr_t)

enum {
    WILINK = 0,
    WIENTR = 1,
    WIBODY = 2,
    WINAME = 3,
    WIATTR = 4,
};

enum {
    WOLINK = 0,
    WOENTR = WIENTR * ADDRSIZE,
    WOBODY = WIBODY * ADDRSIZE,
    WONAME = WINAME * ADDRSIZE,
    WOATTR = WIATTR * ADDRSIZE,
    WHSIZE = WOATTR + ADDRSIZE,
};

enum {
    WATTMASK = (0xFF << 8),
    WATTIMMD = (1 << 8),
    WLENMASK = (0xFF),
};

#define rodef(label, name, link, entry, body, attr) \
const char _str_##label[] = name; \
const void *f_##label[] = { \
[WILINK] = (void *)link, \
[WIENTR] = (void *)entry, \
[WIBODY] = (void *)body, \
[WINAME] = (void *)&_str_##label[0], \
[WIATTR] = (void *)(((sizeof(_str_##label) -1) & WLENMASK) | (attr & WATTMASK)) \
};

rodef(dummy, "dummy", NULL, NULL, NULL, 0);

#define rodefcode(label, name, link, attr) \
void c_##label(void); \
rodef(label, name, link, c_##label, -1, attr); \
void c_##label(void)

rodefcode(bye, "bye", f_dummy, 0) {
    exit(EXIT_SUCCESS);
}

enum {
    RINO = 0,
    RISP = 1,
    RIRP = 2,
    RIIP = 3,
    RIWP = 4,
    RIXP = 5,
    RISB = 6,
    RIST = 7,
    RIRB = 8,
    RIRT = 9,
    RISS = 10,
    RINP = 11,
};

enum {
    ROSP = RISP * ADDRSIZE,
    RORP = RIRP * ADDRSIZE,
    ROIP = RIIP * ADDRSIZE,
    ROWP = RIWP * ADDRSIZE,
    ROXP = RIXP * ADDRSIZE,
    ROSB = RISB * ADDRSIZE,
    ROST = RIST * ADDRSIZE,
    RORB = RIRB * ADDRSIZE,
    RORT = RIRT * ADDRSIZE,
    ROSS = RISS * ADDRSIZE,
    RONP = RINP * ADDRSIZE,
};

enum {
    SSCOMP = (1 << 8),
    SSDOVE = (1 << 9),
    SSROVE = (1 << 10),
    SSDUND = (1 << 11),
    SSRUND = (1 << 12),
};

#define STKGAP 8
#define regsnew(label, gap, dsize, rsize, entry, link) \
void *dstk_##label[gap + dsize + gap]; \
void *rstk_##label[gap + rsize + gap]; \
void *regs_##label[] = { \
[RISP] = (void *)&dstk_##label[gap], \
[RIRP] = (void *)&rstk_##label[gap], \
[RIIP] = (void *)entry, \
[RIWP] = (void *)0, \
[RIXP] = (void *)0, \
[RISB] = (void *)&dstk_##label[gap], \
[RIST] = (void *)&dstk_##label[gap + dsize - 1], \
[RIRB] = (void *)&rstk_##label[gap], \
[RIRT] = (void *)&rstk_##label[gap + rsize - 1], \
[RISS] = (void *)0, \
[RINP] = (void *)link, \
};

void *up = NULL;

#define LOAD(addr) (*(intptr_t *)(addr))
#define STORE(v, addr) (*(intptr_t *)(addr) = (intptr_t)(v))
#define CLOAD(addr) (*(uint8_t *)(addr))
#define CSTORE(v, addr) (*(uint8_t *)(addr) = (uint8_t)(v))

#define UPGET() (LOAD(&up))
#define UPSET(v) (STORE((v), &up))

#define IPGET() (LOAD(UPGET() + ROIP))
#define WPGET() (LOAD(UPGET() + ROWP))
#define XPGET() (LOAD(UPGET() + ROXP))
#define IPSET(v) (STORE((v), (UPGET() + ROIP)))
#define WPSET(v) (STORE((v), (UPGET() + ROWP)))
#define XPSET(v) (STORE((v), (UPGET() + ROXP)))

void (*next_jump)(void);

rodefcode(next, "next", f_bye, 0) {
    WPSET(LOAD(IPGET()));
    IPSET(IPGET() + ADDRSIZE);
    next_jump = (void *)(LOAD(WPGET() + WOENTR));
    return next_jump();
}

#define NEXT() return c_next()

#define RPGET() (LOAD(UPGET() + RORP))
#define RPSET(v) (STORE((v), (UPGET() + RORP)))
#define RBGET() (LOAD(UPGET() + RORB))
#define RTGET() (LOAD(UPGET() + RORT))
#define SSGET() (LOAD(UPGET() + ROSS))
#define SSSET(v) (STORE((v), (UPGET() + ROSS)))

void rpush(intptr_t v) {
    if (RPGET() > RTGET()) {
        SSSET(SSGET() | SSROVE);
    }
    STORE(v ,RPGET());
    RPSET(RPGET() + ADDRSIZE);
    if (RPGET() > RTGET()) {
        SSSET(SSGET() | SSROVE);
    }
}

intptr_t rpop(void) {
    RPSET(RPGET() - ADDRSIZE);
    if (RPGET() < RBGET()) {
        SSSET(SSGET() | SSRUND);
        return -1;
    }
    return LOAD(RPGET());
}

rodefcode(call, "call", f_next, 0) {
    rpush(IPGET());
    IPSET(LOAD(WPGET() + WOBODY));
    NEXT();
}

rodefcode(exit, "exit", f_call, 0) {
    IPSET(rpop());
    NEXT();
}

#define rodefword(label, name, link, attr) \
const void *w_##label[]; \
rodef(label, name, link, c_call, w_##label, attr); \
const void *w_##label[] =

rodefword(nop, "nop", f_exit, 0) {
    f_exit,
};

#define SPGET() (LOAD(UPGET() + ROSP))
#define SPSET(v) (STORE((v), (UPGET() + ROSP)))
#define SBGET() (LOAD(UPGET() + ROSB))
#define STGET() (LOAD(UPGET() + ROST))

void dpush(intptr_t v) {
    if (SPGET() > STGET()) {
        SSSET(SSGET() | SSDOVE);
        return;
    }
    STORE(v ,SPGET());
    SPSET(SPGET() + ADDRSIZE);
    if (SPGET() > STGET()) {
        SSSET(SSGET() | SSDOVE);
        return;
    }
}

intptr_t dpop(void) {
    SPSET(SPGET() - ADDRSIZE);
    if (SPGET() < SBGET()) {
        SSSET(SSGET() | SSDUND);
    }
    return LOAD(SPGET());
}

rodefcode(lit, "lit", f_nop, 0) {
    dpush(LOAD(IPGET()));
    IPSET(IPGET() + ADDRSIZE);
    NEXT();
}

rodefcode(branch0, "branch0", f_lit, 0) {
    WPSET(dpop());
    XPSET(LOAD(IPGET()));
    IPSET(IPGET() + ADDRSIZE);
    if (WPGET() == 0) {
        IPSET(XPGET());
    }
    NEXT();
}

rodefcode(branch, "branch", f_branch0, 0) {
    IPSET(LOAD(IPGET()));
    NEXT();
}

rodefcode(execute, "execute", f_branch, 0) {
    WPSET(dpop());
    next_jump = (void *)(LOAD(WPGET() + WOENTR));
    return next_jump();
}

rodefcode(xor, "^", f_execute, 0) {
    dpush(dpop() ^ dpop());
    NEXT();
}

rodefword(false, "false", f_xor, 0) {
    f_lit, 0, f_exit,
};

rodefword(true, "true", f_false, 0) {
    f_lit, -1, f_exit,
};

const void *w_equ_true[] = {
    f_true, f_exit,
};

rodefword(equ, "=", f_true, 0) {
    f_xor, f_branch0, w_equ_true,
    f_false, f_exit,
};

rodefcode(or, "|", f_equ, 0) {
    dpush(dpop() | dpop());
    NEXT();
}

rodefcode(and, "&", f_or, 0) {
    dpush(dpop() & dpop());
    NEXT();
}

rodefword(invert, "invert", f_and, 0) {
    f_lit, -1, f_xor, f_exit,
};

rodefcode(add, "+", f_invert, 0) {
    dpush(dpop() + dpop());
    NEXT();
}

rodefword(inc, "1+", f_add, 0) {
    f_lit, 1, f_add, f_exit,
};

rodefword(negate, "negate", f_inc, 0) {
    f_invert, f_inc, f_exit,
};

rodefword(sub, "-", f_negate, 0) {
    f_negate, f_add, f_exit,
};

rodefword(dec, "1-", f_sub, 0) {
    f_lit, 1, f_sub, f_exit,
};

rodefcode(lshift, "<<", f_dec, 0) {
    WPSET(dpop());
    dpush(dpop() << WPGET());
    NEXT();
}

rodefcode(rshift, "<<", f_lshift, 0) {
    WPSET(dpop());
    dpush(dpop() >> WPGET());
    NEXT();
}

rodefword(bic, "bic", f_rshift, 0) {
    f_invert, f_and, f_exit,
};

rodefcode(load, "@", f_bic, 0) {
    dpush(LOAD(dpop()));
    NEXT();
}

rodefcode(store, "!", f_load, 0) {
    WPSET(dpop());
    STORE(dpop(), WPGET());
    NEXT();
}

rodefcode(cload, "c@", f_store, 0) {
    dpush(CLOAD(dpop()));
    NEXT();
}

rodefcode(cstore, "c!", f_cload, 0) {
    WPSET(dpop());
    CSTORE(dpop(), WPGET());
    NEXT();
}

rodefcode(depth, "depth", f_cstore, 0) {
    if (ADDRSIZE == 8) {
        dpush((SPGET() - SBGET()) >> 3);
    }
    if (ADDRSIZE == 4) {
        dpush((SPGET() - SBGET()) >> 2);
    }
    NEXT();
}

rodefword(dchk, "dchk", f_depth, 0) {
    f_depth, f_lit, 0, f_equ, f_branch0, -1,
    f_exit,
};

rodefcode(drop, "drop", f_dchk, 0) {
    dpop();
    NEXT();
}

rodefcode(dup, "dup", f_drop, 0) {
    WPSET(dpop());
    dpush(WPGET());
    dpush(WPGET());
    NEXT();
}

rodefcode(swap, "swap", f_dup, 0) {
    WPSET(dpop());
    XPSET(dpop());
    dpush(WPGET());
    dpush(XPGET());
    NEXT();
}

rodefcode(tor, ">r", f_swap, 0) {
    rpush(dpop());
    NEXT();
}

rodefcode(fromr, "r>", f_tor, 0) {
    dpush(rpop());
    NEXT();
}

rodefword(over, "over", f_fromr, 0) {
    f_tor, f_dup, f_fromr, f_swap, f_exit,
};

rodefword(rot, "rot", f_over, 0) {
    f_tor, f_swap, f_fromr, f_swap, f_exit,
};

rodefcode(2lit, "2lit", f_rot, 0) {
    dpush(LOAD(IPGET()));
    IPSET(IPGET() + ADDRSIZE);
    dpush(LOAD(IPGET()));
    IPSET(IPGET() + ADDRSIZE);
    NEXT();
}

rodefword(2drop, "2drop", f_2lit, 0) {
    f_drop, f_drop, f_exit,
};

rodefword(2dup, "2dup", f_2drop, 0) {
    f_over, f_over, f_exit,
};

rodefword(2swap, "2swap", f_2dup, 0) {
    f_rot, f_tor, f_rot, f_fromr, f_exit,
};

rodefword(2over, "2over", f_2swap, 0) {
    f_tor, f_tor, f_2dup, f_fromr, f_fromr, f_2swap, f_exit,
};

rodefword(2rot, "2rot", f_2over, 0) {
    f_tor, f_tor, f_2swap, f_fromr, f_fromr, f_2swap, f_exit,
};

rodefword(eqz, "0=", f_2rot, 0) {
    f_lit, 0, f_equ, f_exit,
};

rodefword(neq, "<>", f_eqz, 0) {
    f_equ, f_invert, f_exit,
};

rodefword(nez, "0<>", f_neq, 0) {
    f_eqz, f_invert, f_exit,
};

rodefcode(doconst, "doconst", f_nez, 0) {
    dpush(LOAD(WPGET() + WOBODY));
    NEXT();
};

#define rodefconst(label, name, value, link, attr) \
rodef(label, name, link, c_doconst, value, attr);

rodefconst(cell, "cell", ADDRSIZE, f_doconst, 0);

const void *w_ltz_nosupport[] = {
    f_branch, -1,
};

const void *w_ltz_false[] = {
    f_false, f_exit,
};

const void *w_ltz_4[] = {
    f_cell, f_lit, 4, f_equ, f_branch0, w_ltz_nosupport,
    f_lit, 1, f_lit, 31, f_lshift, f_and, f_branch0, w_ltz_false,
    f_true, f_exit,
};

rodefword(ltz, "0<", f_cell, 0) {
    f_cell, f_lit, 8, f_equ, f_branch0, w_ltz_4,
    f_lit, 1, f_lit, 63, f_lshift, f_and, f_branch0, w_ltz_false,
    f_true, f_exit,
};

rodefword(gez, "0>=", f_ltz, 0) {
    f_ltz, f_invert, f_exit,
};

rodefword(gtz, "0>", f_gez, 0) {
    f_dup, f_gez, f_swap, f_nez, f_and, f_exit,
};

rodefword(lez, "0<=", f_gtz, 0) {
    f_gtz, f_invert, f_exit,
};

const void *w_lt_false[] = {
    f_false, f_exit,
};

rodefword(lt, "<", f_lez, 0) {
    f_sub, f_ltz, f_branch0, w_lt_false,
    f_true, f_exit,
};

rodefword(gt, ">", f_lt, 0) {
    f_swap, f_lt, f_exit,
};

rodefword(le, "<=", f_gt, 0) {
    f_gt, f_invert, f_exit,
};

rodefword(ge, ">=", f_le, 0) {
    f_lt, f_invert, f_exit,
};

const void *w_min_top[] = {
    f_swap, f_drop, f_exit,
};

rodefword(min, "min", f_ge, 0) {
    f_2dup, f_lt, f_branch0, w_min_top,
    f_drop, f_exit,
};

const void *w_max_top[] = {
    f_swap, f_drop, f_exit,
};

rodefword(max, "max", f_min, 0) {
    f_2dup, f_gt, f_branch0, w_max_top,
    f_drop, f_exit,
};

rodefword(within, "within", f_max, 0) {
    f_2dup, // n min max min max
    f_min, // n min max min
    f_rot, // n max min min
    f_rot, // n min min max
    f_max, // n min max
    f_rot, // min max n
    f_dup, // min max n n
    f_rot, // min n n max
    f_lt,  // min n n<max
    f_rot, // n n<max min
    f_rot, // n<max min n
    f_le,  // n<max min<=n
    f_and,
    f_exit,
};

rodefcode(emit, "emit", f_within, 0) {
    dpop();
    while(write(STDOUT_FILENO, SPGET(), 1) != 1);
    NEXT();
}

const void *w_type_end[] = {
    f_2drop, f_exit,
};

rodefword(type, "type", f_emit, 0) {
    f_dup, f_branch0, w_type_end,
    f_dec, f_swap, f_dup, f_cload, f_emit,
    f_inc, f_swap, f_branch, w_type,
};

const void *w_types_end[] = {
    f_2drop, f_exit,
};

rodefword(types, "types", f_type, 0) {
    f_dup, f_cload, &f_dup, f_branch0, w_types_end,
    f_emit, f_inc, f_branch, w_types,
};

uint8_t xdigits[] = "0123456789ABCDEF";

rodefword(hex4, "hex4", f_types, 0) {
    f_lit, 0xF, f_and,
    f_lit, xdigits, f_add, f_cload, f_emit, f_exit,
};

rodefword(hex8, "hex8", f_hex4, 0) {
    f_dup, f_lit, 4, f_rshift, f_hex4, f_hex4, f_exit,
};

rodefword(hex16, "hex16", f_hex8, 0) {
    f_dup, f_lit, 8, f_rshift, f_hex8, f_hex8, f_exit,
};

rodefword(hex32, "hex32", f_hex16, 0) {
    f_dup, f_lit, 16, f_rshift, f_hex16, f_hex16, f_exit,
};

rodefword(hex64, "hex64", f_hex32, 0) {
    f_dup, f_lit, 32, f_rshift, f_hex32, f_hex32, f_exit,
};

const void *w_hex_8[] = {
    f_hex8, f_exit,
};

const void *w_hex_16[] = {
    f_cell, f_lit, 2, f_equ, f_branch0, w_hex_8,
    f_hex16, f_exit,
};

const void *w_hex_32[] = {
    f_cell, f_lit, 4, f_equ, f_branch0, w_hex_16,
    f_hex32, f_exit,
};

rodefword(hex, "hex", f_hex64, 0) {
    f_cell, f_lit, 8, f_equ, f_branch0, w_hex_32,
    f_hex64, f_exit,
};

rodefword(isxdigit, "isxdigit", f_hex, 0) {
    f_dup, f_2lit, '0', '9' + 1, f_within,
    f_swap, f_2lit, 'A', 'F' + 1, f_within,
    f_or, f_exit,
};

const void *w_isnumber_false[] = {
    f_2drop, f_false, f_exit,
};

const void *w_isnumber_true[] = {
    f_2drop, f_true, f_exit,
};

const void *w_isnumber_loop[] = {
    f_dup, f_branch0, w_isnumber_true,
    f_dec, f_over, f_cload, f_isxdigit,
    f_branch0, w_isnumber_false,
    f_swap, f_inc, f_swap,
    f_branch, w_isnumber_loop,
};

// addr u
rodefword(isnumber, "isnumber", f_isxdigit, 0) {
    f_dup, f_branch0, w_isnumber_false,
    f_branch, w_isnumber_loop,
};

const void *w_hex2num_x[] = {
    f_lit, 0xA, f_add, f_lit, 'A', f_sub,
    f_exit,
};

rodefword(hex2num, "hex2num", f_isnumber, 0) {
    f_dup, f_2lit, '0', '9' + 1, f_within,
    f_branch0, w_hex2num_x,
    f_lit, '0', f_sub, f_exit,
};

const void *w_number_end[] = {
    f_2drop, f_swap, f_drop, f_exit,
};

const void *w_number_loop[] = {
    f_2swap, f_dup, f_branch0, w_number_end,
    f_swap, f_dup, f_cload, f_hex2num, f_tor,
    f_inc, f_swap, f_dec, f_2swap,
    f_swap, f_fromr, f_over, f_lshift,
    f_tor, f_lit, 4, f_sub, f_swap,
    f_fromr, f_or,
    f_branch, w_number_loop,
};

// addr u
rodefword(number, "number", f_hex2num, 0) {
    f_dup, f_dec, f_lit, 2, f_lshift, // mul 4
    f_lit, 0,
    f_branch, w_number_loop,
};

rodefcode(spget, "sp@", f_hex2num, 0) {
    dpush(SPGET());
    NEXT();
};

rodefcode(sbget, "sb@", f_spget, 0) {
    dpush(SBGET());
    NEXT();
};

rodefword(newline, "newline", f_sbget, 0) {
    f_lit, "\n\r", f_types, f_exit,
};

const void *w_dsdump_end[] = {
    f_2drop, f_newline, f_exit,
};

const void *w_dsdump_loop[] = {
    f_2dup, f_lt, f_branch0, w_dsdump_end,
    f_over, f_load,
    f_lit, ' ', f_emit,
    f_hex,
    f_lit, ' ', f_emit,
    f_swap, f_cell, f_add, f_swap,
    f_branch, w_dsdump_loop,
};

rodefword(dsdump, ".s", f_sbget, 0) {
    f_spget, f_sbget, f_swap,
    f_depth, f_lit, 2, f_sub,
    f_lit, '<', f_emit,
    f_hex8,
    f_lit, '>', f_emit,
    f_branch, w_dsdump_loop,
};

const void *w_compare_ok[] = {
    f_drop, f_2drop, f_true, f_exit,
};

const void *w_compare_fail[] = {
    f_drop, f_2drop, f_false, f_exit,
};

const void *w_compare_loop[] = {
    f_dup, f_branch0, w_compare_ok,
    f_dec, f_rot, f_rot,
    f_2dup, f_cload, f_swap, f_cload,
    f_equ, f_branch0, w_compare_fail,
    f_inc, f_swap, f_inc, f_rot,
    f_branch, w_compare_loop,
};

// addr0 u0 addr1 u1
rodefword(compare, "compare", f_dsdump, 0) {
    f_rot, f_min,
    f_dup, f_branch0, w_compare_fail,
    f_branch, w_compare_loop,
};

const void *w_cmove_end[] = {
    f_drop, f_2drop, f_exit,
};

const void *w_fill_end[] = {
    f_drop, f_2drop, f_exit,
};

// addr0 u c
rodefword(fill, "fill", f_compare, 0) {
    f_over, f_branch0, w_fill_end,
    f_swap, f_dec, f_tor,
    f_swap,  f_2dup, f_cstore,
    f_inc, f_swap, f_fromr, f_swap,
    f_branch, w_fill,
};

// addr0 addr1 u
rodefword(cmove, "cmove", f_fill, 0) {
    f_dup, f_branch0, w_cmove_end,
    f_tor, f_over, f_cload, f_over, f_cstore,
    f_inc, f_swap, f_inc, f_swap, f_fromr, f_dec,
    f_branch, w_cmove,
};

const void *w_strlen_end[] = {
    f_drop, f_exit,
};

const void *w_strlen_loop[] = {
    f_dup, f_cload, f_branch0, w_strlen_end,
    f_inc, f_swap, f_inc, f_swap,
    f_branch, w_strlen_loop,
};

// addr0
rodefword(strlen, "strlen", f_cmove, 0) {
    f_lit, 0, f_swap,
    f_branch, w_strlen_loop,
};

void *latest;
uint8_t dict[32 * 1024 * 1024];
void *here = dict;

rodefconst(latest, "latest", &latest, f_strlen, 0);
rodefconst(here, "here", &here, f_latest, 0);
rodefconst(wolink, "wolink", WOLINK, f_here, 0);
rodefconst(woentr, "woentr", WOENTR, f_wolink, 0);
rodefconst(wobody, "wobody", WOBODY, f_woentr, 0);
rodefconst(woname, "woname", WONAME, f_wobody, 0);
rodefconst(woattr, "woattr", WOATTR, f_woname, 0);
rodefconst(wattmask, "wattmask", WATTMASK, f_woattr, 0);
rodefconst(wlenmask, "wlenmask", WLENMASK, f_wattmask, 0);
rodefconst(whsize, "whsize", WHSIZE, f_wlenmask, 0);

rodefword(latestget, "latest@", f_whsize, 0) {
    f_latest, f_load, f_exit,
};

rodefword(latestset, "latest!", f_latestget, 0) {
    f_latest, f_store, f_exit,
};

rodefword(hereget, "here@", f_latestset, 0) {
    f_here, f_load, f_exit,
};

rodefword(hereset, "here!", f_hereget, 0) {
    f_here, f_store, f_exit,
};

rodefword(wlinkget, "wlink@", f_hereset, 0) {
    f_wolink, f_add, f_load, f_exit,
};

rodefword(wlinkset, "wlink!", f_wlinkget, 0) {
    f_wolink, f_add, f_store, f_exit,
};

rodefword(wentrget, "wentr@", f_wlinkset, 0) {
    f_woentr, f_add, f_load, f_exit,
};

rodefword(wentrset, "wentr!", f_wentrget, 0) {
    f_woentr, f_add, f_store, f_exit,
};

rodefword(wbodyget, "wbody@", f_wentrset, 0) {
    f_wobody, f_add, f_load, f_exit,
};

rodefword(wbodyset, "wbody!", f_wbodyget, 0) {
    f_wobody, f_add, f_store, f_exit,
};

rodefword(wnameget, "wname@", f_wbodyset, 0) {
    f_woname, f_add, f_load, f_exit,
};

rodefword(wnameset, "wname!", f_wnameget, 0) {
    f_woname, f_add, f_store, f_exit,
};

rodefword(wattrget, "wattr@", f_wnameset, 0) {
    f_woattr, f_add, f_load,
    f_wattmask, &f_and, f_exit,
};

rodefword(wattrset, "wattr!", f_wattrget, 0) {
    f_dup, f_woattr, f_add, f_load,
    f_wattmask, f_bic, f_rot, f_or,
    f_swap, f_woattr, f_add, f_store,
    f_exit,
};

rodefconst(wattimmd, "wattimmd", WATTIMMD, f_wattrset, 0);

const void *w_wisimmd_false[] = {
    f_false, f_exit,
};

rodefword(wisimmd, "wisimmd", f_wattimmd, 0) {
    f_wattrget, f_wattimmd, f_and, f_branch0, w_wisimmd_false,
    f_true, f_exit,
};

rodefword(wnlenget, "wnlen@", f_wisimmd, 0) {
    f_woattr, f_add, f_load,
    f_wlenmask, f_and, f_exit,
};

rodefword(wnlenset, "wnlen!", f_wnlenget, 0) {
    f_dup, f_woattr, f_add, f_load,
    f_wlenmask, f_bic, f_rot, f_or,
    f_swap, f_woattr, f_add, f_store,
    f_exit,
};

const void *w_words_end[] = {
    f_newline, f_drop, f_exit,
};

const void *w_words_loop[] = {
    f_dup, f_branch0, w_words_end,
    f_dup, f_wnameget, f_over, f_wnlenget,
    f_type,
    f_lit, ' ', f_emit,
    f_wlinkget, f_branch, w_words_loop,
};

rodefword(words, "words", f_wnlenset, 0) {
    f_latestget,
    f_newline,
    f_branch, w_words_loop,
};

const void *w_find_fail[] = {
    f_drop, f_2drop, f_false, f_exit,
};

const void *w_find_loop[];

const void *w_find_next[] = {
    f_wlinkget, f_branch, w_find_loop,
};

const void *w_find_loop[] = {
    f_dup, f_branch0, w_find_fail,
    // addr u waddr
    f_dup, f_wnlenget,
    f_tor, f_over, f_fromr, // addr u waddr u nlen
    f_equ, f_branch0, w_find_next,
    f_dup, f_wnameget, // addr u waddr wname
    f_2over, // addr u waddr wname addr u
    f_rot, f_over, // addr u waddr addr u wname u
    f_compare, f_branch0, w_find_next,
    f_tor, f_2drop, f_fromr, f_exit,
};

// addr u
rodefword(find, "find", f_words, 0) {
    f_latestget,
    f_branch, w_find_loop,
};

// addr u
rodefword(defword, "defword", f_find, 0) {
    f_latestget, f_hereget, f_wlinkset,
    f_2lit, "call", 4, f_find, f_wentrget, f_hereget, f_wentrset, f_hereget, f_wnlenset,
    f_hereget, f_whsize, f_add, f_hereget, f_wnameset,
    f_hereget, f_wnameget, f_hereget, f_wnlenget, f_add, f_hereget, f_wbodyset,
    f_hereget, f_wnameget, f_hereget, f_wnlenget, f_cmove,
    f_lit, 0, f_hereget, f_wattrset,
    f_hereget, f_latestset, f_hereget, f_wbodyget, f_hereset,
    f_exit,
};

intptr_t stdinbuf = 0;
#define STDIN_RECV_OK (1 << 8)

rodefcode(keyava, "key?", f_defword, 0) {
    if (stdinbuf & STDIN_RECV_OK) {
        dpush(-1);
        NEXT();
    }
    stdinbuf = 0;
    if (read(STDIN_FILENO, &stdinbuf, 1) == 1) {
        stdinbuf |= STDIN_RECV_OK;
        dpush(-1);
        NEXT();
    }
    dpush(0);
    NEXT();
}

#define NPGET() (LOAD(UPGET() + RONP))

rodefcode(yield, "yield", f_keyava, 0) {
    UPSET(NPGET());
    NEXT();
}

rodefconst(stdinbuf, "stdinbuf", &stdinbuf, f_yield, 0);
rodefconst(stdin_recv_ok, "stdin_recv_ok", STDIN_RECV_OK, f_stdinbuf, 0);

rodefword(waitkey, "waitkey", f_stdin_recv_ok, 0) {
    f_yield, f_keyava, f_branch0, w_waitkey, f_exit,
};

rodefword(key, "key", f_waitkey, 0) {
    f_waitkey, f_stdinbuf, f_load,
    f_stdin_recv_ok, f_xor, f_dup, f_stdinbuf, f_store,
    f_exit,
};

uint8_t tib[128];
void *tip = &tib[0];

rodefconst(tib, "tib", &tib[0], f_key, 0);
rodefconst(tip, "tip", &tip, f_tib, 0);
rodefconst(tit, "tit", &tib[127], f_tip, 0);

rodefword(tipget, "tip@", f_tit, 0) {
    f_tip, f_load, f_exit,
};

rodefword(tipset, "tip!", f_tipget, 0) {
    f_tip, f_store, f_exit,
};

rodefword(tiused, "tiused", f_tipset, 0) {
    f_tipget, f_tib, f_sub, f_exit,
};

rodefword(tipchk, "tipchk", f_tipset, 0) {
    f_tipget, f_tib, f_tit, f_within, f_exit,
};

rodefword(tiprst, "tiprst", f_tipchk, 0) {
    f_tib, f_tipset, f_exit,
};

const void *w_tip_ove_und[] = {
    f_tiprst, f_exit,
};

rodefword(tipinc, "tip1+", f_tiprst, 0) {
    f_tipget, f_inc, f_tipset,
    f_tipchk, f_branch0, w_tip_ove_und,
    f_exit,
};

rodefword(tipdec, "tip1-", f_tipinc, 0) {
    f_tipget, f_dec, f_tipset,
    f_tipchk, f_branch0, w_tip_ove_und,
    f_exit,
};

rodefword(tipush, "tipush", f_tipdec, 0) {
    f_tipget, f_cstore, f_tipinc,
    f_exit,
};

rodefword(tipop, "tipop", f_tipush, 0) {
    f_tipdec, f_tipget, f_cload,
    f_exit,
};

rodefconst(cr, "cr", '\r', f_tipop, 0);
rodefconst(lf, "lf", '\n', f_cr, 0);

rodefword(isnewline, "isnewline", f_lf, 0) {
    f_dup, f_cr, f_equ, f_swap, f_lf, f_equ, f_or, f_exit,
};

rodefconst(space, "space", ' ', f_isnewline, 0);
rodefconst(tab, "tab", '\t', f_space, 0);

rodefword(isspace, "isspace", f_tab, 0) {
    f_dup, f_space, f_equ, f_swap, f_tab, f_equ, f_or, f_exit,
};

rodefconst(backspace, "backspace", '\b', f_isspace, 0);
rodefconst(del, "del", 0x7F, f_backspace, 0);

rodefword(isdel, "isdel", f_del, 0) {
    f_dup, f_backspace, f_equ, f_swap, f_del, f_equ, f_or, f_exit,
};

const void *w_getword_end[] = {
    f_drop, f_exit,
};

const void *w_getword_nl[] = {
    f_newline, f_branch, w_getword_end,
};

const void *w_getword[];

const void *w_getword_delchar[] = {
    f_drop, f_tipop, f_drop,
    f_lit, "\x1B[1D \x1B[1D", f_types,
    f_branch, w_getword,
};

rodefword(getword, "getword", f_isspace, 0) {
    f_key, f_dup, f_emit,
    f_dup, f_isspace, f_invert, f_branch0, w_getword_end,
    f_dup, f_isnewline, f_invert, f_branch0, w_getword_nl,
    f_dup, f_isdel, f_invert, f_branch0, w_getword_delchar,
    f_tipush, f_branch, w_getword,
};

rodefcode(dsrst, "dsrst", f_getword, 0) {
    SPSET(SBGET());
    NEXT();
};

rodefcode(ssget, "ss@", f_dsrst, 0) {
    dpush(SSGET());
    NEXT();
};

rodefcode(ssset, "ss!", f_ssget, 0) {
    SSSET(dpop());
    NEXT();
};

rodefconst(ssdove, "ssdove", SSDOVE, f_ssset, 0);
rodefconst(ssdund, "ssdund", SSDUND, f_ssdove, 0);
rodefconst(ssrove, "ssrove", SSROVE, f_ssdund, 0);
rodefconst(ssrund, "ssrund", SSRUND, f_ssrove, 0);

const void *w_sschk_end[] = {
    f_exit,
};

const void *w_sschk_rund[] = {
    f_ssget, f_ssrund, f_and, f_branch0, w_sschk_end,
    f_newline,
    f_lit, "return stack underflow", f_types,
    f_drop, f_false,
    f_newline,
    f_branch, w_sschk_end,
};

const void *w_sschk_rove[] = {
    f_ssget, f_ssrove, f_and, f_branch0, w_sschk_rund,
    f_newline,
    f_lit, "return stack overflow", f_types,
    f_drop, f_false,
    f_newline,
    f_branch, w_sschk_rund,
};


const void *w_sschk_dund[] = {
    f_ssget, f_ssdund, f_and, f_branch0, w_sschk_rove,
    f_newline,
    f_lit, "data stack underflow", f_types,
    f_drop, f_false,
    f_newline,
    f_branch, w_sschk_rove,
};

const void *w_sschk_dove[] = {
    f_ssget, f_ssdove, f_and, f_branch0, w_sschk_dund,
    f_newline,
    f_lit, "data stack overflow", f_types,
    f_drop, f_false,
    f_newline,
    f_branch, w_sschk_dund,
};

rodefword(sschk, "sschk", f_ssrund, 0) {
    f_true,
    f_branch, w_sschk_dove,
};

rodefconst(sscomp, "sscomp", SSCOMP, f_sschk, 0);

const void *w_compstat_false[] = {
    f_false, f_exit,
};

rodefword(compstat, "compstat", f_sscomp, 0) {
    f_ssget, f_sscomp, f_and, f_branch0, w_compstat_false,
    f_true, f_exit,
};

rodefword(compon, "]", f_compstat, 0) {
    f_ssget, f_sscomp, f_or, f_ssset, f_exit,
};

rodefword(compoff, "[", f_compon, WATTIMMD) {
    f_ssget, f_sscomp, f_bic, f_ssset, f_exit,
};

rodefword(docon, ":", f_compoff, 0) {
    f_tiprst, f_getword, f_tiused, f_branch0, w_docon,
    f_tib, f_tiused, f_defword, f_compon, f_exit,
};

rodefword(semcol, ";", f_docon, WATTIMMD) {
    f_2lit, "exit", 4, f_find, f_hereget, f_store,
    f_hereget, f_cell, f_add, f_hereset,
    f_compoff, f_exit,
};

rodefword(ifnz, "if", f_semcol, WATTIMMD) {
    f_2lit, "branch0", 7, f_find, f_hereget, f_store,
    f_hereget, f_cell, f_add, f_hereset,
    f_lit, -1,  f_hereget, f_store,
    f_hereget, f_dup, f_cell, f_add, f_hereset,
    f_exit,
};

rodefword(then, "then", f_ifnz, WATTIMMD) {
    f_hereget, f_swap, f_store,
    f_exit,
};

// : testif if 111 exit then 222 ;

rodefword(begin, "begin", f_then, WATTIMMD) {
    f_hereget, f_exit,
};

rodefword(until, "until", f_begin, WATTIMMD) {
    f_2lit, "branch0", 7, f_find, f_hereget, f_store,
    f_hereget, f_cell, f_add, f_hereset,
    f_hereget, f_store,
    f_hereget, f_cell, f_add, f_hereset,
    f_exit,
};

// : star 2A emit ;
// : stars dup if begin star 1- dup 0= until then drop ;

const void *w_interpret[];

const void *w_interpret_nonumber[] = {
    f_tib, f_tiused, f_type,
    f_lit, " not found", f_types,
    f_newline,
    f_dsrst,
    f_branch, w_interpret,
};

const void *w_interpret_noword[] = {
    f_drop, f_tib, f_tiused, f_isnumber, f_branch0, w_interpret_nonumber,
    f_tib, f_tiused, f_number, f_compstat, f_branch0, w_interpret,
    f_2lit, "lit", 3, f_find, f_hereget, f_store,
    f_hereget, f_cell, f_add, f_hereset,
    f_hereget, f_store,
    f_hereget, f_cell, f_add, f_hereset,
    f_branch, w_interpret,
};

const void *w_interpret_ssclear[] = {
    f_ssget, f_ssdove, f_bic, f_ssset,
    f_ssget, f_ssdund, f_bic, f_ssset,
    f_ssget, f_ssrove, f_bic, f_ssset,
    f_ssget, f_ssrund, f_bic, f_ssset,
    f_dsrst,
    f_branch, w_interpret,
};

const void *w_interpret_compile[] = {
    f_hereget, f_store,
    f_hereget, f_cell, f_add, f_hereset,
    f_branch, w_interpret,
};

const void *w_interpret_execute[] = {
    f_execute, f_sschk, f_branch0, w_interpret_ssclear,
    f_branch, w_interpret,
};

rodefword(interpret, "interpret", f_until, 0) {
    f_tiprst, f_getword, f_tiused, f_branch0, w_interpret,
    f_tib, f_tiused, f_find, f_dup, f_branch0, w_interpret_noword,
    f_compstat, f_branch0, w_interpret_execute,
    f_dup, f_wisimmd, f_branch0, w_interpret_compile,
    f_branch, w_interpret_execute,
};

void *latest = f_interpret;

const void *test_getword[] = {
    f_tiprst,
    f_getword,
    f_lit, '[', f_emit,
    f_tib, f_tipget, f_over, f_sub, f_type,
    f_lit, ']', f_emit,
    f_branch, test_getword,
};

const void *test_lit_branch0_nop_bye[] = {
    f_dchk, f_lit, -1, f_branch0, -1, f_nop, f_dchk,
    //f_branch, test_getword,
    f_interpret,
    f_bye, -1,
};

const void *test_emit_type_types_words[] = {
    f_dchk,
    f_newline,
    f_lit, 'W', f_emit,
    f_lit, "elcome", f_lit, 6, f_type,
    f_lit, " Wizard", f_types,
    f_newline,
    f_words,
    f_dchk,
    f_branch, test_lit_branch0_nop_bye,
};

const void *test_dsdump[] = {
    f_dchk,
    f_dsdump,
    f_2lit, 0, 1, f_2lit, 2, 3, f_2lit, 4, 5,
    f_dsdump,
    f_2drop,
    f_dsdump,
    f_2drop,
    f_dsdump,
    f_2drop,
    f_dsdump,
    f_dchk,
    f_branch, test_emit_type_types_words
};

const void *test_hex[] = {
    f_dchk,
    f_lit, 0, f_hex4,
    f_lit, 1, f_hex4,
    f_lit, 9, f_hex4,
    f_lit, 0xA, f_hex4,
    f_lit, 0xB, f_hex4,
    f_lit, 0xF, f_hex4,
    f_lit, 0x0, f_hex4,
    f_lit, 0x01, f_hex8,
    f_lit, 0x10, f_hex8,
    f_lit, 0x0F, f_hex8,
    f_lit, 0xF0, f_hex8,
    f_lit, 0xFF, f_hex8,
    f_lit, 0x01, f_hex8,
    f_lit, 0x0101, f_hex16,
    f_lit, 0x1010, f_hex16,
    f_lit, 0x0F0F, f_hex16,
    f_lit, 0xF0F0, f_hex16,
    f_lit, 0xFFFF, f_hex16,
    f_lit, 0x0123, f_hex16,
    f_lit, 0x01010101, f_hex32,
    f_lit, 0x10101010, f_hex32,
    f_lit, 0x0F0F0F0F, f_hex32,
    f_lit, 0xF0F0F0F0, f_hex32,
    f_lit, 0xFFFFFFFF, f_hex32,
    f_lit, 0x01234567, f_hex32,
    f_lit, 0x0101010101010101, f_hex64,
    f_lit, 0x1010101010101010, f_hex64,
    f_lit, 0x0F0F0F0F0F0F0F0F, f_hex64,
    f_lit, 0xF0F0F0F0F0F0F0F0, f_hex64,
    f_lit, 0xFFFFFFFFFFFFFFFF, f_hex64,
    f_lit, 0x0123456789ABCDEF, f_hex64,
    f_lit, 0x0123456789ABCDEF, f_hex,
    f_dchk,
    f_branch, test_dsdump,
};

const void *test_branch0[] = {
    f_lit, "ok\n\r", f_types,
    f_dchk, f_lit, 0, f_branch0, test_hex,
};

const void *test_execute[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "execute test: ", f_types,
    f_dchk,
    f_lit, f_nop, f_execute,
    f_lit, f_cell, f_execute, f_cell, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_branch0,
};

const void *test_xor0[] = {
    f_dchk, f_lit, 1, f_lit, 1, f_xor, f_branch0, test_execute,
};

const void *test_xor[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "xor test: ", f_types,
    f_dchk,
    f_lit, 1, f_lit, 0, f_xor, f_branch0, -1,
    f_dchk,
    f_lit, 0, f_lit, 1, f_xor, f_branch0, -1,
    f_dchk,
    f_lit, 1, f_lit, 1, f_xor, f_branch0, test_xor0,
};

const void *test_equ[] = {
    f_lit, "equ test: ", f_types,
    f_dchk,
    f_lit, 1, f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 0, f_lit, 0, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 0, f_lit, 1, f_equ, f_false, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 1, f_lit, 0, f_equ, f_branch0, test_xor,
};

const void *test_or[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "or test: ", f_types,
    f_dchk,
    f_lit, 1, f_lit, 1, f_or, f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 1, f_lit, 0, f_or, f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 0, f_lit, 1, f_or, f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 0, f_lit, 0, f_or, f_branch0, test_equ,
};

const void *test_and[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "and test: ", f_types,
    f_dchk,
    f_lit, 1, f_lit, 1, f_and, f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 0, f_lit, 1, f_and, f_lit, 0, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 1, f_lit, 0, f_and, f_lit, 0, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 0, f_lit, 0, f_and, f_branch0, test_or,
};

const void *test_invert[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "invert test: ", f_types,
    f_dchk,
    f_lit, 0, f_invert, f_lit, -1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, -1, f_invert, f_branch0, test_and,
};

const void *test_add[] ={
    f_lit, "ok\n\r", f_types,
    f_lit, "add test: ", f_types,
    f_dchk,
    f_lit, 0, f_lit, 1, f_add, f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 1, f_lit, 0, f_add, f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 1, f_lit, 1, f_add, f_lit, 2, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, -1, f_lit, -1, f_add, f_lit, -2, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 1, f_lit, -1, f_add, f_branch0, test_invert,
};

const void *test_inc[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "inc test: ", f_types,
    f_dchk,
    f_lit, 0, f_inc, f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 1, f_inc, f_lit, 2, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, -2, f_inc, f_lit, -1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, -1, f_inc, f_branch0, test_add,
};

const void *test_negate[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "negate test: ", f_types,
    f_dchk,
    f_lit, 1, f_negate, f_lit, -1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 2, f_negate, f_lit, -2, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, -2, f_negate, f_lit, 2, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, -1, f_negate, f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 0, f_negate, f_branch0, test_inc,
};

const void *test_sub[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "sub test: ", f_types,
    f_dchk,
    f_lit, 0, f_lit, 5, f_sub, f_lit, -5, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 5, f_lit, 0, f_sub, f_lit, 5, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 0, f_lit, -5, f_sub, f_lit, 5, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, -5, f_lit, 0, f_sub, f_lit, -5, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 1, f_lit, 2, f_sub, f_lit, -1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 1, f_lit, -2, f_sub, f_lit, 3, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, -1, f_lit, 2, f_sub, f_lit, -3, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, -1, f_lit, -2, f_sub, f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 1, f_lit, 0, f_sub, f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 0, f_lit, 1, f_sub, f_lit, -1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 0, f_lit, 0, f_sub, f_lit, 0, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, -1, f_lit, -1, f_sub, f_lit, 0, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 1, f_lit, 1, f_sub, f_branch0, test_negate,
};

const void *test_dec[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "dec test: ", f_types,
    f_dchk,
    f_lit, 0, f_dec, f_lit, -1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, -1, f_dec, f_lit, -2, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 2, f_dec, f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 1, f_dec, f_branch0, test_sub,
};

const void *test_lshift[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "lshift test: ", f_types,
    f_dchk,
    f_lit, 1, f_lit, 0, f_lshift, f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 1, f_lit, 1, f_lshift, f_lit, 2, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 0, f_lit, 1, f_lshift, f_branch0, test_dec,
};

const void *test_rshift[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "rshift test: ", f_types,
    f_dchk,
    f_lit, 1, f_lit, 0, f_rshift, f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 2, f_lit, 1, f_rshift, f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 0, f_lit, 1, f_rshift, f_branch0, test_lshift,
};

const void *test_bic[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "bic test: ", f_types,
    f_dchk,
    f_lit, 0xFF, f_lit, 0x80, f_bic, f_lit, 0x7F, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 0xFF, f_lit, 0x81, f_bic, f_lit, 0x7E, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 1, f_lit, 1, f_bic, f_branch0, test_lshift,
};

intptr_t test_buf0[128];
intptr_t test_buf1[128];

const void *test_load_store[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "load store test: ", f_types,
    f_dchk,
    f_lit, -1, f_lit, test_buf0, f_store,
    f_dchk,
    f_lit, test_buf0, f_load, f_lit, -1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 0, f_lit, test_buf0, f_store,
    f_dchk,
    f_lit, test_buf0, f_load, f_branch0, test_bic,
};

const void *test_cload_cstore[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "cload cstore test: ", f_types,
    f_dchk,
    f_lit, 0x55, f_lit, test_buf0, f_cstore,
    f_dchk,
    f_lit, test_buf0, f_cload, f_lit, 0x55, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 0x00, f_lit, test_buf0, f_cstore,
    f_dchk,
    f_lit, test_buf0, f_cload, f_branch0, test_load_store,
};

const void *test_depth[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "depth test: ", f_types,
    f_dchk,
    f_lit, 1, f_depth, f_lit, 1, f_equ, f_branch0, -1,
    f_lit, 1, f_depth, f_lit, 2, f_equ, f_branch0, -1, f_add, f_lit, 2, f_equ, f_branch0, -1,
    f_dchk,
    f_depth, f_branch0, test_cload_cstore,
};

const void *test_drop[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "drop test: ", f_types,
    f_dchk,
    f_lit, 0, f_drop,
    f_dchk,
    f_lit, 0, f_lit, 2, f_drop, f_drop,
    f_dchk,
    f_branch, test_depth,
};

const void *test_dup[] ={
    f_lit, "ok\n\r", f_types,
    f_lit, "dup test: ", f_types,
    f_dchk,
    f_lit, 1, f_dup,
    f_lit, 1, f_equ, f_branch0, -1,
    f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_drop,
};

const void *test_swap[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "swap test: ", f_types,
    f_dchk,
    f_lit, 1, f_lit, 2,
    f_lit, 2, f_equ, f_branch0, -1,
    f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_lit, 1, f_lit, 2, f_swap,
    f_lit, 1, f_equ, f_branch0, -1,
    f_lit, 2, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_dup,
};

const void *test_tor_fromr[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "tor fromr test: ", f_types,
    f_dchk,
    f_lit, 1, f_lit, 2, f_tor, f_tor,
    f_fromr,  f_lit, 1, f_equ, f_branch0, -1,
    f_fromr,  f_lit, 2, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_swap,
};

const void *test_over[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "over test: ", f_types,
    f_dchk,
    f_lit, 1, f_lit, 2, f_over,
    f_lit, 1, f_equ, f_branch0, -1,
    f_lit, 2, f_equ, f_branch0, -1,
    f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_tor_fromr,
};

const void *test_rot[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "rot test: ", f_types,
    f_dchk,
    f_lit, 1, f_lit, 2, f_lit, 3, f_rot,
    f_lit, 1, f_equ, f_branch0, -1,
    f_lit, 3, f_equ, f_branch0, -1,
    f_lit, 2, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_over,
};

const void *test_2lit[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "2lit test: ", f_types,
    f_dchk,
    f_2lit, 1, 2,
    f_lit, 2, f_equ, f_branch0, -1,
    f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_rot,
};

const void *test_2drop[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "2drop test: ", f_types,
    f_dchk,
    f_2lit, 1, 2, f_2drop,
    f_dchk,
    f_branch, test_2lit,
};

const void *test_2dup[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "2dup test: ", f_types,
    f_dchk,
    f_2lit, 1, 2, f_2dup,
    f_lit, 2, f_equ, f_branch0, -1,
    f_lit, 1, f_equ, f_branch0, -1,
    f_lit, 2, f_equ, f_branch0, -1,
    f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_2drop,
};

const void *test_2swap[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "2swap test: ", f_types,
    f_dchk,
    f_2lit, 1 , 2, f_2lit, 3, 4, f_2swap,
    f_lit, 2, f_equ, f_branch0, -1,
    f_lit, 1, f_equ, f_branch0, -1,
    f_lit, 4, f_equ, f_branch0, -1,
    f_lit, 3, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_2dup,
};

const void *test_2over[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "2over test: ", f_types,
    f_dchk,
    f_2lit, 1, 2, f_2lit, 3, 4, f_2over,
    f_lit, 2, f_equ, f_branch0, -1,
    f_lit, 1, f_equ, f_branch0, -1,
    f_lit, 4, f_equ, f_branch0, -1,
    f_lit, 3, f_equ, f_branch0, -1,
    f_lit, 2, f_equ, f_branch0, -1,
    f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_2swap,
};

const void *test_2rot[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "2rot test: ", f_types,
    f_dchk,
    f_2lit, 1, 2, f_2lit, 3, 4, f_2lit, 5, 6, f_2rot,
    f_lit, 2, f_equ, f_branch0, -1,
    f_lit, 1, f_equ, f_branch0, -1,
    f_lit, 6, f_equ, f_branch0, -1,
    f_lit, 5, f_equ, f_branch0, -1,
    f_lit, 4, f_equ, f_branch0, -1,
    f_lit, 3, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_2over,
};

const void *test_eqz[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "eqz test: ", f_types,
    f_dchk,
    f_lit, 0, f_eqz, f_branch0, -1,
    f_lit, 1, f_eqz, f_false, f_equ, f_branch0, -1,
    f_lit, -1, f_eqz, f_false, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_2rot,
};

const void *test_neq[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "neq test: ", f_types,
    f_dchk,
    f_lit, 0, f_lit, 1, f_neq, f_branch0, -1,
    f_lit, 1, f_lit, 0, f_neq, f_branch0, -1,
    f_lit, 0, f_lit, 0, f_neq, f_false, f_equ, f_branch0, -1,
    f_lit, 1, f_lit, 1, f_neq, f_false, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_eqz,
};

const void *test_nez[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "nez test: ", f_types,
    f_dchk,
    f_lit, 1, f_nez, f_branch0, -1,
    f_lit, -1, f_nez, f_branch0, -1,
    f_lit, 0, f_nez, f_false, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_neq,
};

const void *test_cell[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "cell test: ", f_types,
    f_dchk,
    f_cell, f_lit, ADDRSIZE, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_nez,
};

const void *test_ltz[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "ltz test: ", f_types,
    f_dchk,
    f_lit, -1, f_ltz, f_branch0, -1,
    f_lit, -2, f_ltz, f_branch0, -1,
    f_lit, -5, f_ltz, f_branch0, -1,
    f_lit, 0, f_ltz, f_false, f_equ, f_branch0, -1,
    f_lit, 1, f_ltz, f_false, f_equ, f_branch0, -1,
    f_lit, 2, f_ltz, f_false, f_equ, f_branch0, -1,
    f_lit, 5, f_ltz, f_false, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_cell,
};

const void *test_gez[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "gez test: ", f_types,
    f_dchk,
    f_lit, 0, f_gez, f_branch0, -1,
    f_lit, 1, f_gez, f_branch0, -1,
    f_lit, 2, f_gez, f_branch0, -1,
    f_lit, 5, f_gez, f_branch0, -1,
    f_lit, -1, f_gez, f_false, f_equ, f_branch0, -1,
    f_lit, -2, f_gez, f_false, f_equ, f_branch0, -1,
    f_lit, -5, f_gez, f_false, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_ltz
};

const void *test_gtz[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "gtz test: ", f_types,
    f_dchk,
    f_lit, 1, f_gtz, f_branch0, -1,
    f_lit, 2, f_gtz, f_branch0, -1,
    f_lit, 5, f_gtz, f_branch0, -1,
    f_lit, 0, f_gtz, f_false, f_equ, f_branch0, -1,
    f_lit, -1, f_gtz, f_false, f_equ, f_branch0, -1,
    f_lit, -2, f_gtz, f_false, f_equ, f_branch0, -1,
    f_lit, -5, f_gtz, f_false, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_gez,
};

const void *test_lez[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "lez test: ", f_types,
    f_dchk,
    f_lit, 0, f_lez, f_branch0, -1,
    f_lit, -1, f_lez, f_branch0, -1,
    f_lit, -2, f_lez, f_branch0, -1,
    f_lit, -5, f_lez, f_branch0, -1,
    f_lit, 1, f_lez, f_false, f_equ, f_branch0, -1,
    f_lit, 2, f_lez, f_false, f_equ, f_branch0, -1,
    f_lit, 5, f_lez, f_false, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_gtz,
};

const void *test_lt[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "lt test: ", f_types,
    f_dchk,
    f_lit, 0, f_lit, 1, f_lt, f_branch0, -1,
    f_lit, 1, f_lit, 2, f_lt, f_branch0, -1,
    f_lit, -1, f_lit, 0, f_lt, f_branch0, -1,
    f_lit, -1, f_lit, 1, f_lt, f_branch0, -1,
    f_lit, 0, f_lit, 0, f_lt, f_false, f_equ, f_branch0, -1,
    f_lit, 1, f_lit, 1, f_lt, f_false, f_equ, f_branch0, -1,
    f_lit, 1, f_lit, 0, f_lt, f_false, f_equ, f_branch0, -1,
    f_lit, 2, f_lit, 1, f_lt, f_false, f_equ, f_branch0, -1,
    f_lit, 0, f_lit, -1, f_lt, f_false, f_equ, f_branch0, -1,
    f_lit, 1, f_lit, -1, f_lt, f_false, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_lez,
};

const void *test_gt[] =  {
    f_lit, "ok\n\r", f_types,
    f_lit, "gt test: ", f_types,
    f_dchk,
    f_lit, 1, f_lit, 0, f_gt, f_branch0, -1,
    f_lit, 2, f_lit, 1, f_gt, f_branch0, -1,
    f_lit, 3, f_lit, 2, f_gt, f_branch0, -1,
    f_lit, 0, f_lit, -1, f_gt, f_branch0, -1,
    f_lit, -1, f_lit, -2, f_gt, f_branch0, -1,
    f_lit, 1, f_lit, -1, f_gt, f_branch0, -1,
    f_lit, 0, f_lit, 1, f_gt, f_false, f_equ, f_branch0, -1,
    f_lit, 0, f_lit, 0, f_gt, f_false, f_equ, f_branch0, -1,
    f_lit, 1, f_lit, 1, f_gt, f_false, f_equ, f_branch0, -1,
    f_lit, -1, f_lit, -1, f_gt, f_false, f_equ, f_branch0, -1,
    f_lit, -1, f_lit, 0, f_gt, f_false, f_equ, f_branch0, -1,
    f_lit, -2, f_lit, -1, f_gt, f_false, f_equ, f_branch0, -1,
    f_lit, -1, f_lit, 1, f_gt, f_false, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_lt,
};

const void *test_le[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "le test: ", f_types,
    f_dchk,
    f_lit, 0, f_lit, 1, f_le, f_branch0, -1,
    f_lit, 0, f_lit, 0, f_le, f_branch0, -1,
    f_lit, 1, f_lit, 1, f_le, f_branch0, -1,
    f_lit, 1, f_lit, 2, f_le, f_branch0, -1,
    f_lit, -1, f_lit, 0, f_le, f_branch0, -1,
    f_lit, -1, f_lit, 1, f_le, f_branch0, -1,
    f_lit, -1, f_lit, -1, f_le, f_branch0, -1,
    f_lit, 2, f_lit, 1, f_le, f_false, f_equ, f_branch0, -1,
    f_lit, 1, f_lit, 0, f_le, f_false, f_equ, f_branch0, -1,
    f_lit, 0, f_lit, -1, f_le, f_false, f_equ, f_branch0, -1,
    f_lit, -1, f_lit, -2, f_le, f_false, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_gt,
};

const void *test_ge[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "ge test: ", f_types,
    f_dchk,
    f_lit, 1, f_lit, 0, f_ge, f_branch0, -1,
    f_lit, 0, f_lit, 0, f_ge, f_branch0, -1,
    f_lit, 1, f_lit, 1, f_ge, f_branch0, -1,
    f_lit, 2, f_lit, 1, f_ge, f_branch0, -1,
    f_lit, 0, f_lit, -1, f_ge, f_branch0, -1,
    f_lit, -1, f_lit, -2, f_ge, f_branch0, -1,
    f_lit, 1, f_lit, -1, f_ge, f_branch0, -1,
    f_lit, 0, f_lit, 1, f_ge, f_false, f_equ, f_branch0, -1,
    f_lit, 1, f_lit, 2, f_ge, f_false, f_equ, f_branch0, -1,
    f_lit, -1, f_lit, 0, f_ge, f_false, f_equ, f_branch0, -1,
    f_lit, -1, f_lit, 1, f_ge, f_false, f_equ, f_branch0, -1,
    f_lit, -2, f_lit, -1, f_ge, f_false, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_le,
};

const void *test_min[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "min test: ", f_types,
    f_dchk,
    f_lit, 0, f_lit, 1, f_min, f_lit, 0, f_equ, f_branch0, -1,
    f_lit, 1, f_lit, 0, f_min, f_lit, 0, f_equ, f_branch0, -1,
    f_lit, 1, f_lit, 2, f_min, f_lit, 1, f_equ, f_branch0, -1,
    f_lit, 2, f_lit, 1, f_min, f_lit, 1, f_equ, f_branch0, -1,
    f_lit, 0, f_lit, -1, f_min, f_lit, -1, f_equ, f_branch0, -1,
    f_lit, -1, f_lit, 0, f_min, f_lit, -1, f_equ, f_branch0, -1,
    f_lit, 1, f_lit, -1, f_min, f_lit, -1, f_equ, f_branch0, -1,
    f_lit, -1, f_lit, 1, f_min, f_lit, -1, f_equ, f_branch0, -1,
    f_lit, -2, f_lit, -1, f_min, f_lit, -2, f_equ, f_branch0, -1,
    f_lit, -1, f_lit, -2, f_min, f_lit, -2, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_ge,
};

const void *test_max[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "max test: ", f_types,
    f_dchk,
    f_lit, 0, f_lit, 1, f_max, f_lit, 1, f_equ, f_branch0, -1,
    f_lit, 1, f_lit, 0, f_max, f_lit, 1, f_equ, f_branch0, -1,
    f_lit, -1, f_lit, 0, f_max, f_lit, 0, f_equ, f_branch0, -1,
    f_lit, 0, f_lit, -1, f_max, f_lit, 0, f_equ, f_branch0, -1,
    f_lit, -1, f_lit, -2, f_max, f_lit, -1, f_equ, f_branch0, -1,
    f_lit, -2, f_lit, -1, f_max, f_lit, -1, f_equ, f_branch0, -1,
    f_lit, 1, f_lit, -1, f_max, f_lit, 1, f_equ, f_branch0, -1,
    f_lit, -1, f_lit, 1, f_max, f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_min,
};

const void *test_within[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "within test: ", f_types,
    f_dchk,
    f_lit, 1, f_2lit, 1, 3, f_within, f_branch0, -1,
    f_lit, 2, f_2lit, 1, 3, f_within, f_branch0, -1,
    f_lit, 3, f_2lit, 1, 3, f_within, f_false, f_equ, f_branch0, -1,
    f_lit, 0, f_2lit, 1, 3, f_within, f_false, f_equ, f_branch0, -1,
    f_lit, -1, f_2lit, 1, 3, f_within, f_false, f_equ, f_branch0, -1,
    f_lit, 4, f_2lit, 1, 3, f_within, f_false, f_equ, f_branch0, -1,
    f_lit, 1, f_2lit, 1, 1, f_within, f_false, f_equ, f_branch0, -1,
    f_lit, 1, f_2lit, 3, 1, f_within, f_branch0, -1,
    f_lit, 2, f_2lit, 3, 1, f_within, f_branch0, -1,
    f_lit, 3, f_2lit, 3, 1, f_within, f_false, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_max,
};

const char cmpa[] = "Hello Alice";
const char cmpb[] = "Hello World";
const int cmplen = sizeof(cmpa);

const void *test_compare[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "compare test: ", f_types,
    f_dchk,
    f_2lit, cmpa, 5, f_2lit, cmpb, 5, f_compare, f_branch0, -1,
    f_2lit, cmpa, 7, f_2lit, cmpb, 5, f_compare, f_branch0, -1,
    f_2lit, cmpa, 6, f_2lit, cmpb, 6, f_compare, f_branch0, -1,
    f_2lit, cmpa, 7, f_2lit, cmpb, 7, f_compare, f_false, f_equ, f_branch0, -1,
    f_2lit, cmpa, 0, f_2lit, cmpb, 0, f_compare, f_false, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_within,
};

//intptr_t test_buf0[128];
//intptr_t test_buf1[128];

const void *test_fill_cmove[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "fill cmove test: ", f_types,
    f_dchk,
    f_2lit, test_buf0, 8, f_lit, 0x55, f_fill,
    f_2lit, test_buf0, test_buf1, f_lit, 8, f_cmove,
    f_2lit, test_buf0, 8, f_2lit, test_buf1, 8, f_compare, f_branch0, -1,
    f_2lit, test_buf0, 7, f_lit, 0xAA, f_fill,
    f_2lit, test_buf0, test_buf1, f_lit, 7, f_cmove,
    f_2lit, test_buf0, 7, f_2lit, test_buf1, 7, f_compare, f_branch0, -1,
    f_dchk,
    f_branch, test_compare
};

const void *test_strlen[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "strlen test: ", f_types,
    f_dchk,
    f_lit, "", f_strlen, f_lit, 0, f_equ, f_branch0, -1,
    f_lit, "1", f_strlen, f_lit, 1, f_equ, f_branch0, -1,
    f_lit, "55", f_strlen, f_lit, 2, f_equ, f_branch0, -1,
    f_lit, "123", f_strlen, f_lit, 3, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_fill_cmove,
};

const void *test_find[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "find test: ", f_types,
    f_dchk,
    f_2lit, "nop", 3, f_find, f_lit, f_nop, f_equ, f_branch0, -1,
    f_2lit, "noa", 3, f_find, f_false, f_equ, f_branch0, -1,
    f_2lit, "abcdefghijk", 11, f_find, f_false, f_equ, f_branch0, -1,
    f_2lit, "next", 4, f_find, f_lit, f_next, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_strlen,
};

void *wtest_word[] =  {
    [WILINK] = -1,
    [WIENTR] = -2,
    [WIBODY] = -3,
    [WINAME] = -4,
    [WIATTR] = 0x55 | 0x55 << 8,
};

const void *test_wutils[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "wutils test: ", f_types,
    f_dchk,
    f_lit, test_buf0,
    f_lit, -1, f_over, f_wlinkset,
    f_dup, f_wlinkget, f_lit, -1, f_equ, f_branch0, -1,
    f_lit, -2, f_over, f_wentrset,
    f_dup, f_wentrget, f_lit, -2, f_equ, f_branch0, -1,
    f_lit, -3, f_over, f_wbodyset,
    f_dup, f_wbodyget, f_lit, -3, f_equ, f_branch0, -1,
    f_lit, -4, f_over, f_wnameset,
    f_dup, f_wnameget, f_lit, -4, f_equ, f_branch0, -1,
    f_lit, 0xFF, f_over, f_wnlenset,
    f_dup, f_wnlenget, f_lit, 0xFF, f_equ, f_branch0, -1,
    f_lit, 0x55, f_over, f_wnlenset,
    f_dup, f_wnlenget, f_lit, 0x55, f_equ, f_branch0, -1,
    f_lit, 0xFF << 8, f_over, f_wattrset,
    f_dup, f_wattrget, f_lit, 0xFF << 8, f_equ, f_branch0, -1,
    f_lit, 0x55 << 8, f_over, f_wattrset,
    f_dup, f_wattrget, f_lit, 0x55 << 8, f_equ, f_branch0, -1,
    f_lit, sizeof(wtest_word), f_lit, wtest_word, f_over,
    f_compare, f_branch0, -1,
    f_lit, f_compoff, f_wisimmd, f_branch0, -1,
    f_dchk,
    f_branch, test_find,
};

const void *test_defword[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "defword test: ", f_types,
    f_dchk,
    f_2lit, "1", 1, f_defword,
    f_2lit, "lit", 3, f_find, f_hereget, f_store,
    f_hereget, f_cell, f_add, f_hereset,
    f_lit, 1, f_hereget, f_store,
    f_hereget, f_cell, f_add, f_hereset,
    f_2lit, "exit", 4, f_find, f_hereget, f_store,
    f_hereget, f_cell, f_add, f_hereset,
    f_2lit, "1", 1, f_find, f_execute, f_lit, 1, f_equ, f_branch0, -1,
    f_dchk,
    f_2lit, "0", 1, f_defword,
    f_2lit, "lit", 3, f_find, f_hereget, f_store,
    f_hereget, f_cell, f_add, f_hereset,
    f_lit, 0, f_hereget, f_store,
    f_hereget, f_cell, f_add, f_hereset,
    f_2lit, "exit", 4, f_find, f_hereget, f_store,
    f_hereget, f_cell, f_add, f_hereset,
    f_2lit, "0", 1, f_find, f_execute, f_lit, 0, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_wutils,
};

const void *test_isxdigit[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "isxdigit test: ", f_types,
    f_dchk,
    f_lit, 'A', f_isxdigit, f_branch0, -1,
    f_lit, 'F', f_isxdigit, f_branch0, -1,
    f_lit, '0', f_isxdigit, f_branch0, -1,
    f_lit, '9', f_isxdigit, f_branch0, -1,
    f_lit, '0'-1, f_isxdigit, f_false, f_equ, f_branch0, -1,
    f_lit, '9'+1, f_isxdigit, f_false, f_equ, f_branch0, -1,
    f_lit, 'A'-1, f_isxdigit, f_false, f_equ, f_branch0, -1,
    f_lit, 'F'+1, f_isxdigit, f_false, f_equ, f_branch0, -1,
    f_lit, ' ', f_isxdigit, f_false, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_defword,
};

const void *test_isnumber[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "isnumber test: ", f_types,
    f_dchk,
    f_2lit, "", 0, f_isnumber,f_false, f_equ, f_branch0, -1,
    f_2lit, "1", 1, f_isnumber, f_branch0, -1,
    f_2lit, "123", 3, f_isnumber, f_branch0, -1,
    f_2lit, " 12", 3, f_isnumber,f_false, f_equ, f_branch0, -1,
    f_2lit, "   ", 3, f_isnumber,f_false, f_equ, f_branch0, -1,
    f_2lit, "9 8", 3, f_isnumber,f_false, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_isxdigit,
};

const void *test_hex2num[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "hex2num test: ", f_types,
    f_dchk,
    f_lit, '0', f_hex2num, f_lit, 0, f_equ, f_branch0, -1,
    f_lit, '9', f_hex2num, f_lit, 9, f_equ, f_branch0, -1,
    f_lit, 'A', f_hex2num, f_lit, 0xA, f_equ, f_branch0, -1,
    f_lit, 'F', f_hex2num, f_lit, 0xF, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_isnumber,
};

const void *test_number[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "number test: ", f_types,
    f_dchk,
    f_2lit, "", 0, f_number, f_lit, 0, f_equ, f_branch0, -1,
    f_2lit, "1", 1, f_number, f_lit, 1, f_equ, f_branch0, -1,
    f_2lit, "12", 2, f_number, f_lit, 0x12, f_equ, f_branch0, -1,
    f_2lit, "123", 3, f_number, f_lit, 0x123, f_equ, f_branch0, -1,
    f_2lit, "1234", 4, f_number, f_lit, 0x1234, f_equ, f_branch0, -1,
    f_2lit, "01234567", 8, f_number, f_lit, 0x01234567, f_equ, f_branch0, -1,
    f_2lit, "FEDCBA98", 8, f_number, f_lit, 0xFEDCBA98, f_equ, f_branch0, -1,
    f_dchk,
    f_cell, f_lit, 8, f_equ, f_branch0, test_hex2num,
    f_2lit, "0123456789ABCDEF", 16, f_number, f_lit, 0x0123456789ABCDEF, f_equ, f_branch0, -1,
    f_2lit, "FEDCBA9876543210", 16, f_number, f_lit, 0xFEDCBA9876543210, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_hex2num,
};

const void *test_tib_tip_tit[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "tib tip tit test: ", f_types,
    f_dchk,
    f_tib, f_lit, &tib[0], f_equ, f_branch0, -1,
    f_tip, f_lit, &tip, f_equ, f_branch0, -1,
    f_tit, f_lit, &tib[127], f_equ, f_branch0, -1,
    f_tipchk, f_branch0, -1,
    f_tit, f_tipset,  f_tipchk, f_false, f_equ, f_branch0, -1,
    f_tiprst, f_tipget, f_tib, f_equ, f_branch0, -1, f_tipchk, f_branch0, -1,
    f_tipinc, f_tiused, f_lit, 1, f_equ, f_branch0, -1,
    f_tipdec, f_tiused, f_lit, 0, f_equ, f_branch0, -1,
    f_tipdec, f_tipget, f_tib, f_equ, f_branch0, -1,
    f_tit, f_tipset, f_tipinc, f_tipget, f_tib, f_equ, f_branch0, -1,
    f_lit, 0x55, f_tipush, f_tiused, f_lit, 1, f_equ, f_branch0, -1,
    f_tipop, f_lit, 0x55, f_equ, f_branch0, -1,
    f_tiused, f_lit, 0, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_number,
};

const void *test_isspace[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "isspace test: ", f_types,
    f_dchk,
    f_lit, ' ', f_isspace, f_branch0, -1, f_dchk,
    f_lit, '\t', f_isspace, f_branch0, -1, f_dchk,
    f_lit, '\n', f_isspace, f_false, f_equ, f_branch0, -1, f_dchk,
    f_lit, 'a', f_isspace, f_false, f_equ, f_branch0, -1, f_dchk,
    f_dchk,
    f_branch, test_tib_tip_tit,
};

const void *test_isdel[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "isdel test: ", f_types,
    f_dchk,
    f_lit, '\b', f_isdel, f_branch0, -1, f_dchk,
    f_lit, '\x7F', f_isdel, f_branch0, -1, f_dchk,
    f_lit, '\n', f_isdel, f_false, f_equ, f_branch0, -1, f_dchk,
    f_lit, 'a', f_isdel, f_false, f_equ, f_branch0, -1, f_dchk,
    f_dchk,
    f_branch, test_isspace,
};

const void *test_isnewline[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "isnewline test: ", f_types,
    f_dchk,
    f_lit, '\n', f_isnewline, f_branch0, -1, f_dchk,
    f_lit, '\r', f_isnewline, f_branch0, -1, f_dchk,
    f_lit, '\b', f_isnewline, f_false, f_equ, f_branch0, -1, f_dchk,
    f_lit, 'a', f_isnewline, f_false, f_equ, f_branch0, -1, f_dchk,
    f_dchk,
    f_branch, test_isdel,
};

const void *test_sscomp[] = {
    f_lit, "ok\n\r", f_types,
    f_lit, "sscomp test: ", f_types,
    f_dchk,
    f_sscomp, f_lit, SSCOMP, f_equ, f_branch0, -1,
    f_compstat, f_false, f_equ, f_branch0, -1,
    f_compon, f_compstat, f_branch0, -1,
    f_compoff, f_compstat, f_false, f_equ, f_branch0, -1,
    f_dchk,
    f_branch, test_isnewline,
};

const void *human_boot[] = {
    f_yield,
    f_lit, "branch test: ", f_types,
    f_dchk,
    f_branch, test_sscomp,
};

const void *wdt_boot[] = {
    f_yield,
    f_branch, wdt_boot,
};

void *regs_wdt[];
regsnew(human, 8, 128, 128, human_boot, regs_wdt);
regsnew(wdt, 8, 128, 128, wdt_boot, regs_human);

void forth(void) {
    up = regs_human;
    NEXT();
}

void terminit() {
    int i;
    i = fcntl(STDIN_FILENO, F_GETFL);
    i |= O_NONBLOCK;
    fcntl(STDIN_FILENO, F_SETFL, i);
    struct termios termattr;
    tcgetattr(STDIN_FILENO, &termattr);
    cfmakeraw(&termattr);
    tcsetattr(STDIN_FILENO, TCSANOW, &termattr);
}

int main(void) {
    terminit();
    forth();
    exit(EXIT_FAILURE);
}
