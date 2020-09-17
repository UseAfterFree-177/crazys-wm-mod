#include "interface/CGraphics.h"
#include "CLog.h"

CLog g_LogFile;

int main() {
    CGraphics gfx;
    gfx.InitGraphics("test", 800, 600, 800, 600, false);
}