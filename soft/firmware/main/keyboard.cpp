#include "common.h"
#include "grid_mgr.h"
#include "keyboard.h"

namespace GridManager {

Geometry keyb_combo_eng( Rows() 
    << Row("1234567890")
    << Row("QWERTYUIOP()")(SS_BS,2)
    << Row("ASDFGHJKL:@")(SS_Ent, 3)
    << Row("ZXCVBNM,.?/")(SS_Space, 2)(SS_Lang, 1)
);

Geometry keyb_combo_rus(Rows() 
    << Row("1234567890")
    << Row("ЙЦУКЕНГШЩЗХЪ")(SS_BS,2)
    << Row("ФЫВАПРОЛДЖЭ")(SS_Ent, 3)
    << Row("ЯЧСМИТЬБЮЁ.")(SS_Space, 2)(SS_Lang, 1)
);

Geometry keyb_eng( Rows() 
    << Row("1234567890")
    << Row("QWERTYUIOP")(SS_BS,2)
    << Row("ASDFGHJKL")(SS_Ent, 3)
    << Row("ZXCVBNM.")(SS_Space, 4)
);

Geometry keyb_rus(Rows() 
    << Row("1234567890")
    << Row("ЙЦУКЕНГШЩЗХЪ")(SS_BS,2)
    << Row("ФЫВАПРОЛДЖЭ")(SS_Ent, 3)
    << Row("ЯЧСМИТЬБЮЁ.")(SS_Space, 3)
);

Geometry keyb_digits(Rows() 
    << Row("01234")(SS_BS, 2)
    << Row("56789")(SS_Ent, 2)
);

const char* Keyboard::get_text_dos(const MiniCell& cell)
{
    int id = cell.id;
    if (id < 0x100) {sym_buf[0] = id; sym_buf[1] = 0; return sym_buf;}
    sym_buf[0] = id & 0xFF;
    if (cell.col_span > 1)
    {
        assert(cell.col_span <= std::size(sym_buf)-1);
        if (cell.col_span > 2) memset(sym_buf+1, (id & 0xFF) + 2, cell.col_span-2);
        sym_buf[cell.col_span-1] = (id & 0xFF) + 1;
    }
    sym_buf[cell.col_span] = 0;
    return sym_buf;
}

}
