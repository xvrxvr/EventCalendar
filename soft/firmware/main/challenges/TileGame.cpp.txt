#include "utils.h"
#include "GridMgr.h"

static uint16_t scale;
static uint16_t masks[16];
static uint8_t total;

struct TileGame: public GameDrv {
 TileGame() {games[0]=this;}

 virtual int  get_total_variations() {return 3;}

 // 0 - 2x2 (4)
 // 1 - 3x3 (9)
 // 2 - 4x4 (16)

 virtual void fill_short_name(Msg& M, int var) {M.T("Tl").D(var+2);}

 virtual void fill_full_name(Msg& M, int var) {M.T("Плитка ").D(var+2).T("x").D(var+2);}

 virtual void init(int var) 
  {
   uint16_t range=var+2;
   total=range*range;
   scale=0;

   uint8_t sc[16];
   for(int i=0;i<total;++i) sc[i]=i;
   shuffle(sc,total);

   for(int i=0;i<total;++i)
    {
     int csc=_BV(sc[i]);
     for(int j=i+1;j<total;++j)
      if (rand()&1) csc|=_BV(sc[j]);
     masks[sc[i]]=csc;
    }

   grid.init2(range,0);
   grid.setup_all(0,-1);
   Msg().gotoxy(320,0).bottom().center().T("Зажгите все плитки и ящик откроется!");
  }

 virtual bool tick() // Return true if game is over
  {
   int idx=grid.get_touch_index();
   if (idx==-1) return false;
   int sc=masks[idx];
   scale^=sc;
   for(int i=0;i<total;++i,sc>>=1)
    if (sc&1)
     grid.setup_one(i,(scale&_BV(i))!=0);
   sc=_BV(total)-1;
   return ((scale&sc)==sc);
  }
};

static TileGame title_game;
