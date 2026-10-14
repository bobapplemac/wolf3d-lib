/* Deterministic selection/PCM negotiation tests: no real hardware or DLL. */
#include <stdio.h>
#include <string.h>
#include "../src/WG_OPL.c"
static unsigned attempts[4], destroyed, pcm_opens, pcm_closes;
static uint32_t seen_rate[4], device_rate;
static unsigned failures;
static int pcm_fails;
static void *create(unsigned id, uint32_t rate)
{
    ++attempts[id]; seen_rate[id] = rate;
    if (failures & (1U << id)) { WG_OPL_SetError("fixture unavailable"); return NULL; }
    return (void *)&attempts[id];
}
static void *adlib(uint32_t r) { return create(0,r); }
static void *dbopl(uint32_t r) { return create(1,r); }
static void *nuked(uint32_t r) { return create(2,r); }
static void *silent(uint32_t r) { return create(3,r); }
static void destroy(void *p) { (void)p; ++destroyed; }
static void write_reg(void *p, uint16_t r, uint8_t v) { (void)p;(void)r;(void)v; }
static void generate(void *p, int16_t *s, size_t n) { (void)p;(void)s;(void)n; }
#define DRIVER(fn, label, init) const wg_opl_driver_t *fn(void) { static const wg_opl_driver_t d = {label,0,init,destroy,write_reg,write_reg,generate}; return &d; }
DRIVER(WG_OPL_AdLibDriver,"adlib",adlib)
DRIVER(WG_OPL_DBOPLDriver,"dbopl",dbopl)
DRIVER(WG_OPL_NukedDriver,"nuked",nuked)
DRIVER(WG_OPL_SilentDriver,"silent",silent)
int WG_PCMInit(uint32_t r, uint16_t c, uint32_t *out) { (void)c; ++pcm_opens; *out=device_rate ? device_rate:r; return !pcm_fails; }
void WG_PCMShutdown(void) { ++pcm_closes; }
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"%d: %s\n",__LINE__,#x); return 1; } } while(0)
static void reset(unsigned fail)
{
    memset(attempts,0,sizeof(attempts)); memset(seen_rate,0,sizeof(seen_rate));
    failures=fail; destroyed=pcm_opens=pcm_closes=0; pcm_fails=0; device_rate=0;
    WG_OPL_SelectDriver("auto");
}
int main(void)
{
    wg_opl_t *opl;
    uint32_t rate;
    unsigned preferred = WG_LEGACY_AUDIO ? 1U:2U;
    reset(0); opl=WG_OPL_CreateAudio(0,&rate);
    CHECK(opl && rate==22050 && attempts[0]==1 && pcm_opens==1);
    WG_OPL_Destroy(opl); WG_PCMShutdown(); CHECK(destroyed==1 && pcm_closes==1);
    reset(1); opl=WG_OPL_CreateAudio(0,&rate);
#ifdef TEST_NO_EMULATORS
    CHECK(opl && attempts[3]==1 && strcmp(WG_OPL_SelectedDriver(),"silent")==0);
    CHECK(strstr(WG_OPL_Trace(),"not compiled; skipped") != NULL);
    (void)preferred;
#else
    CHECK(opl && attempts[preferred]==1 && rate==(WG_LEGACY_AUDIO?22050U:48000U));
    CHECK(pcm_opens==2 && pcm_closes==1);
#endif
    WG_OPL_Destroy(opl); WG_PCMShutdown();
    reset(7); opl=WG_OPL_CreateAudio(11025,&rate);
    CHECK(opl && rate==11025 && seen_rate[3]==11025 && attempts[3]==1);
    WG_OPL_Destroy(opl); WG_PCMShutdown();
    reset(1); CHECK(WG_OPL_SelectDriver("adlib"));
    CHECK(!WG_OPL_CreateAudio(0,&rate) && attempts[1]==0 && attempts[2]==0 && attempts[3]==0);
    CHECK(strstr(WG_OPL_Error(),"adlib") && pcm_opens==pcm_closes);
    reset(0); CHECK(!WG_OPL_SelectDriver("missing")); CHECK(!WG_OPL_CreateAudio(0,&rate)); CHECK(!pcm_opens);
#ifdef TEST_NO_EMULATORS
    CHECK(!WG_OPL_SelectDriver("nuked")); CHECK(!WG_OPL_CreateAudio(0,&rate));
#else
    reset(0); CHECK(WG_OPL_SelectDriver("nuked")); opl=WG_OPL_CreateAudio(0,&rate);
    CHECK(opl && !attempts[0] && rate==(WG_LEGACY_AUDIO?44100U:48000U)); WG_OPL_Destroy(opl); WG_PCMShutdown();
#endif
    reset(0); device_rate=21739; opl=WG_OPL_CreateAudio(0,&rate);
    CHECK(opl && rate==21739 && seen_rate[0]==21739); WG_OPL_Destroy(opl); WG_PCMShutdown();
    reset(0); pcm_fails=1; CHECK(!WG_OPL_CreateAudio(0,&rate)); CHECK(pcm_opens==1 && !attempts[0]);
    CHECK(WG_OPL_ParseRate("auto",&rate) && !rate);
    CHECK(WG_OPL_ParseRate("11025",&rate) && rate==11025);
    CHECK(!WG_OPL_ParseRate("0",&rate) && !WG_OPL_ParseRate("7000",&rate));
    CHECK(!WG_OPL_ParseRate("48000junk",&rate) && !WG_OPL_ParseRate("999999999999",&rate));
    puts("Auto/strict OPL selection, rates, cleanup and PCM failures passed."); return 0;
}
