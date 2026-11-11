// Auto no-break: zeroes ForgeChanceRow.destruction (rows) and breakCurve while enabled. F8 toggles; originals are restored when off.
#include <windows.h>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
typedef void* P;
#define F(ret,name,args) typedef ret (__cdecl* name##_t) args; static name##_t name;
F(P,mono_get_root_domain,())
F(P,mono_thread_attach,(P))
F(P,mono_image_loaded,(const char*))
F(P,mono_class_from_name,(P,const char*,const char*))
F(P,mono_class_vtable,(P,P))

static HMODULE g_self; static std::string g_dir;
static bool Rd(const void* a,void* b,size_t n){ __try{ memcpy(b,a,n); return true;}__except(1){return false;} }
template<class T> static bool Rv(const void* a,T& v){ return Rd(a,&v,sizeof(T)); }
template<class T> static bool Wv(void* a,T v){ DWORD o; if(!VirtualProtect(a,sizeof(T),PAGE_READWRITE,&o)) return false; bool ok=true; __try{ *(T*)a=v; }__except(1){ok=false;} VirtualProtect(a,sizeof(T),o,&o); return ok; }
static std::vector<char*> ScanObjs(P vt){ std::vector<char*> r; SYSTEM_INFO si; GetSystemInfo(&si); char* a=(char*)si.lpMinimumApplicationAddress; MEMORY_BASIC_INFORMATION m;
  while(a<(char*)si.lpMaximumApplicationAddress && VirtualQuery(a,&m,sizeof m)){
    if(m.State==MEM_COMMIT&&(m.Protect&(PAGE_READWRITE|PAGE_WRITECOPY))&&!(m.Protect&PAGE_GUARD))
      for(char* p=(char*)m.BaseAddress;p+8<=(char*)m.BaseAddress+m.RegionSize;p+=8){ P v; if(Rv(p,v)&&v==vt) r.push_back(p);} 
    a=(char*)m.BaseAddress+m.RegionSize;} return r; }

static P vtBalance,vtConfig;
struct Target{ char* obj; char* arr; unsigned long long len; };
static Target rows{}, curve{};
static std::map<char*,double> orig; // address -> original value (float rows stored as double)
static bool g_on=true;

static bool RowsOk(const Target& t){ if(!t.obj) return false; P v; if(!Rv(t.obj,v)||v!=vtBalance) return false; char* a; unsigned long long l;
  return Rv(t.obj+0x18,a)&&a==t.arr&&Rv(a+0x18,l)&&l==t.len; }
static bool CurveOk(const Target& t){ if(!t.obj) return false; P v; if(!Rv(t.obj,v)||v!=vtConfig) return false; char* a; unsigned long long l;
  return Rv(t.obj+0x18,a)&&a==t.arr&&Rv(a+0x18,l)&&l==t.len; }
static void FindRows(){ for(char* o:ScanObjs(vtBalance)){ char* a; unsigned long long l; if(!Rv(o+0x18,a)||!a||!Rv(a+0x18,l)||l<2||l>200) continue; bool ok=true;
    for(unsigned long long i=0;i<l&&ok;i++){ int g; ok=Rv(a+0x20+i*24,g)&&g==(int)i; } if(ok){ rows={o,a,l}; return; } } rows={}; }
static void FindCurve(){ for(char* o:ScanObjs(vtConfig)){ char* a; unsigned long long l; if(!Rv(o+0x18,a)||!a||!Rv(a+0x18,l)||l<5||l>200) continue; bool ok=true;
    for(unsigned long long i=0;i<l&&ok;i++){ double d; ok=Rv(a+0x20+i*8,d)&&d>=0&&d<=1; } if(ok){ curve={o,a,l}; return; } } curve={}; }

static void Apply(bool on){
  if(rows.obj) for(unsigned long long i=0;i<rows.len;i++){ char* p=rows.arr+0x20+i*24+4; float v; if(!Rv(p,v)) continue;
    if(on){ if(v!=0){ if(!orig.count(p)) orig[p]=v; Wv<float>(p,0.0f);} } else if(orig.count(p)) { Wv<float>(p,(float)orig[p]); orig.erase(p);} }
  if(curve.obj) for(unsigned long long i=0;i<curve.len;i++){ char* p=curve.arr+0x20+i*8; double v; if(!Rv(p,v)) continue;
    if(on){ if(v!=0){ if(!orig.count(p)) orig[p]=v; Wv<double>(p,0.0);} } else if(orig.count(p)) { Wv<double>(p,orig[p]); orig.erase(p);} } }
static void Status(const char* s){ FILE* f=fopen((g_dir+"status.txt").c_str(),"w"); if(f){ fprintf(f,"ForgeTax No-Break - made by bura\n%s | enabled=%d rows=%s curve=%s\n",s,g_on,rows.obj?"found":"none",curve.obj?"found":"none"); fclose(f);} }

static DWORD WINAPI Run(LPVOID){
  char path[MAX_PATH]; GetModuleFileNameA(g_self,path,MAX_PATH); g_dir=path; g_dir.resize(g_dir.find_last_of('\\')+1);
  HMODULE mono; while(!(mono=GetModuleHandleA("mono-2.0-bdwgc.dll"))) Sleep(500);
#define L(n) n=(n##_t)GetProcAddress(mono,#n); if(!n){Status("missing " #n);return 1;}
  L(mono_get_root_domain)L(mono_thread_attach)L(mono_image_loaded)L(mono_class_from_name)L(mono_class_vtable)
  P dom; while(!(dom=mono_get_root_domain())) Sleep(500); mono_thread_attach(dom);
  P img; while(!(img=mono_image_loaded("Assembly-CSharp"))) Sleep(500);
  P kb,kc; while(!(kb=mono_class_from_name(img,"","ForgeEnhancementBalance"))||!(kc=mono_class_from_name(img,"","ForgeEconomyConfig"))) Sleep(500);
  vtBalance=mono_class_vtable(dom,kb); vtConfig=mono_class_vtable(dom,kc);
  bool prevKey=false; DWORD lastScan=0;
  for(;;){ Sleep(1000);
    bool key=(GetAsyncKeyState(VK_F8)&0x8000)!=0; if(key&&!prevKey){ g_on=!g_on; if(!g_on) Apply(false); } prevKey=key;
    if(!g_on){ Status("off (F8 to enable)"); continue; }
    bool rok=RowsOk(rows), cok=CurveOk(curve);
    if((!rok||!cok)&&GetTickCount()-lastScan>3000){ lastScan=GetTickCount(); if(!rok){ rows={}; FindRows(); } if(!cok){ curve={}; FindCurve(); } }
    Apply(true); Status("running"); } }

BOOL WINAPI DllMain(HINSTANCE h,DWORD r,LPVOID){ if(r==DLL_PROCESS_ATTACH){g_self=h;DisableThreadLibraryCalls(h);CreateThread(0,0,Run,0,0,0);} return TRUE; }
