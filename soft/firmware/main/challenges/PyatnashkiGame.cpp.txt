#include "utils.h"
#include "GridMgr.h"

static uint8_t board[4][4];
static uint8_t range;
static uint8_t hole_x, hole_y;

static int rnd_init(int hole)
{
 int v=rand()%(range-1);
 if (v>=hole) ++v;
 return v;
}

static void upd_tiles(int x1, int y1, int x2, int y2)
{
 for(int x=x1;x<=x2;++x)
  for(int y=y1;y<=y2;++y)
   {
    int v=board[x][y];
    int idx=grid.xy2index(x,y);
    if (v) grid.setup_one(idx,true,Msg().D(v));
    else grid.setup_one(idx,false);
   }
}

static bool hit_tile(int x, int y, bool update_screen=true)
{
 if (x==hole_x && y==hole_y) return false; // we touch empty tile
 if (x!=hole_x && y!=hole_y) return false; // we touch tile that can't be moved
 if (x==hole_x) // move in vertical column
  {
   if (y<hole_y)
    for(int yy=hole_y;yy>y;--yy)
     board[hole_x][yy]=board[hole_x][yy-1];
   else
    for(int yy=hole_y;yy<y;++yy)
     board[hole_x][yy]=board[hole_x][yy+1];
  }
 else
  {
   if (x<hole_x)
    for(int xx=hole_x;xx>x;--xx)
     board[xx][hole_y]=board[xx-1][hole_y];
   else
    for(int xx=hole_x;xx<x;++xx)
     board[xx][hole_y]=board[xx+1][hole_y];   
  }
 board[x][y]=0;
 if (update_screen) upd_tiles(std::min<int>(x,hole_x),std::min<int>(y,hole_y),std::max<int>(x,hole_x),std::max<int>(y,hole_y));
 hole_x=x; hole_y=y;
 return true;
}

struct G15Game: public GameDrv {
 G15Game() {games[1]=this;}

 virtual int  get_total_variations() {return 2;}

 // 0 - 3x3 (8)
 // 1 - 4x4 (15)

 virtual void fill_short_name(Msg& M, int var) {M.T("Пт-").D(var?15:9);}

 virtual void fill_full_name(Msg& M, int var) {M.T(" Пятнашки ").T(var ? "15 (4x4)":"9 (3x3)");}

 virtual void init(int var) 
  {
   range=var+3;

   int i=1;
   for(int y=0;y<range;++y)
    for(int x=0;x<range;++x)
     board[x][y]=i++;
   board[range-1][range-1]=0;
   hole_x=hole_y=range-1;

   for(i=0;i<100;++i)
    {
     int x=hole_x, y=hole_y;
     if (rand()&1) x=rnd_init(x); else y=rnd_init(y);
     hit_tile(x,y,false);
    }

   grid.init2(range,1);
   upd_tiles(0,0,range-1,range-1);
   Msg().gotoxy(320,0).bottom().center().T("Соберите пятнашки и ящик откроется!");
  }

 virtual bool tick() // Return true if game is over
  {
   int idx=grid.get_touch_index();
   if (idx==-1) return false;
   idx=grid.index2xy(idx);
   if (!hit_tile(GET_X(idx),GET_Y(idx))) return false;
   if (hole_x!=range-1 || hole_y!=range-1) return false;
   int i=1,cnt=0;
   for(int y=0;y<range;++y)
    for(int x=0;x<range;++x,++i)
     if (board[x][y]==i) ++cnt;
   return cnt==range*range-1;
  }
};

static G15Game g15_game;
