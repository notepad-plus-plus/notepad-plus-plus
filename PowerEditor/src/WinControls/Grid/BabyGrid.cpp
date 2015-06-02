//BABYGRID code is copyrighted (C) 20002 by David Hillard
//
//This code must retain this copyright message
//
//Printed BABYGRID message reference and tutorial available.
//email: mudcat@mis.net for more information.

/* 
Add WM_MOUSEWHEEL, WM_LBUTTONDBLCLK and WM_RBUTTONUP events
Modified by Don HO <don.h@free.fr>
*/

#include "babygrid.h"
#include "Parameters.h"

#define MAX_GRIDS 20

#define MAX_ROWS 32000
#define MAX_COLS 256


//global variables



HFONT hfontbody,hfontheader,hfonttitle;

HFONT holdfont;

struct _gridhandlestruct 
	{
        UINT gridmenu;
		HWND hlist1;
		TCHAR protect[2];
        TCHAR title[305];
        TCHAR editstring[305];
        TCHAR editstringdisplay[305];
		int rows;
		int cols;
		int gridwidth;
		int gridheight;
		int homerow;
		int homecol;
		int rowheight;
		int leftvisiblecol;
		int rightvisiblecol;
		int topvisiblerow;
		int bottomvisiblerow;
		int headerrowheight;
		int cursorrow;
		int cursorcol;
        int ownerdrawitem;
        int visiblecolumns;
        int titleheight;
        int fontascentheight;
		COLORREF cursorcolor;
		COLORREF protectcolor;
		COLORREF unprotectcolor;
		COLORREF textcolor;
		COLORREF highlightcolor;
		COLORREF gridlinecolor;
        COLORREF highlighttextcolor;
		BOOL DRAWHIGHLIGHT;
        BOOL ADVANCEROW;
        BOOL CURRENTCELLPROTECTED;
		BOOL GRIDHASFOCUS;
        BOOL AUTOROW;
        RECT activecellrect;
        HFONT hfont;
        HFONT hcolumnheadingfont;
        HFONT htitlefont;
		BOOL ROWSNUMBERED;
		BOOL COLUMNSNUMBERED;
        BOOL EDITABLE;
        BOOL EDITING;
        BOOL EXTENDLASTCOLUMN;
        BOOL HSCROLL;
        BOOL VSCROLL;
        BOOL SHOWINTEGRALROWS;
        BOOL SIZING;
        BOOL ELLIPSIS;
        BOOL COLAUTOWIDTH;
        BOOL COLUMNSIZING;
        BOOL ALLOWCOLUMNRESIZING;
        int columntoresize;
        int columntoresizeinitsize;
        int columntoresizeinitx;
        int cursortype;
		int columnwidths[MAX_COLS+1];
        BOOL REMEMBERINTEGRALROWS;
        int wannabeheight;
        int wannabewidth;

    } BGHS[MAX_GRIDS];


_BGCELL BGcell,*LPBGcell;

int BG_GridIndex;
int FindResult;
TCHAR data[1000];







CREATESTRUCT cs,*lpcs;


int         AddGrid(UINT);
int         FindGrid(UINT);
void		ShowVscroll(HWND,int);
void		ShowHscroll(HWND,int);
int         BinarySearchListBox(HWND,TCHAR*);
void        DisplayEditString(HWND ,int ,TCHAR*);
int         CountGrids();




////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
int HomeColumnNthVisible(int SI)
    {  
     int j,hc,count;
     count=0;
     hc=BGHS[SI].homecol;
     for(j=1;j<=hc;++j)
         {
          if(BGHS[SI].columnwidths[j]>0)
              {
               ++count;
              }
         }
     return count;
    }


void RefreshGrid(HWND hWnd)
	{
	 RECT rect;
     int SI;
	 GetClientRect(hWnd,&rect);
	 InvalidateRect(hWnd,&rect,FALSE);
	 SI=FindGrid((UINT)GetMenu(hWnd));
     if(BGHS[SI].EDITING)
         {
          DisplayEditString(hWnd, SI, TEXT(""));
         }

	}

int GetNextColWithWidth(int SI, int startcol, int direction)
    {
     //calls with direction == 1 for right, direction == -1 for left
     //returns 0 if no more cols in that direction, else column number
     int j;
     int ReturnValue;
     j=startcol;
     if(direction == 1){j++;}
     if(direction != 1){j--;}

     while((BGHS[SI].columnwidths[j] == 0)&&(j<=BGHS[SI].cols)&&(j>0))
         {
         if(direction == 1){j++;}
         if(direction != 1){j--;}
         }
     if((BGHS[SI].columnwidths[j] > 0)&&(j<=BGHS[SI].cols))
         {
          ReturnValue = j;
         }
     else
         {
          ReturnValue = 0;
         }
     return ReturnValue;
    }


int GetRowOfMouse(int SI,int y)
	{
	 int ReturnValue;
	 if(y<=(BGHS[SI].titleheight))
		 {
		  return -1;
		 }
	 if((y>=BGHS[SI].titleheight)&&(y<=BGHS[SI].headerrowheight + BGHS[SI].titleheight))
		 {
		  return 0;
		 }


	 y=y-(BGHS[SI].headerrowheight + BGHS[SI].titleheight);
	 y=y/BGHS[SI].rowheight;
	 ReturnValue = BGHS[SI].homerow + y;
	 if(ReturnValue > BGHS[SI].rows){ReturnValue = -1;}
	 return ReturnValue;
	}


int GetColOfMouse(int SI,int x)
	{
	 int ReturnValue;
	 int j;
	 if(x<=BGHS[SI].columnwidths[0])
		 {
		  return 0;
		 }

	 x-=BGHS[SI].columnwidths[0];

	 j=BGHS[SI].homecol;
	 while(x>0)
		 {
		  x-=BGHS[SI].columnwidths[j];
		  j++;
		 }
	 j--;
	 
	 ReturnValue = j;
     if(BGHS[SI].EXTENDLASTCOLUMN)
         {
	      if(j>BGHS[SI].cols){ReturnValue = BGHS[SI].cols;}
         }
     else
         {
          if(j>BGHS[SI].cols){ReturnValue = -1;}
         }
	 return ReturnValue;
	}

BOOL OutOfRange(_BGCELL *cell)
    {
      if((cell->row > MAX_ROWS)||(cell->col > MAX_COLS))
          {return TRUE;}
      else
          {return FALSE;}
    }

void SetCell(_BGCELL *cell,int row, int col)
	{
	 cell->row = row;
	 cell->col = col;
	}

void CalcVisibleCellBoundaries(int SelfIndex)
{
 int gridx,gridy;
 int j;
 gridx=BGHS[SelfIndex].gridwidth;
 gridy=BGHS[SelfIndex].gridheight;



 j=	BGHS[SelfIndex].homecol;
 BGHS[SelfIndex].leftvisiblecol = BGHS[SelfIndex].homecol;
 BGHS[SelfIndex].topvisiblerow = BGHS[SelfIndex].homerow;
 //calc columns visible
 //first subtract the width of col 0;
 gridx = gridx - BGHS[SelfIndex].columnwidths[0];
 do
 {
  gridx = gridx - BGHS[SelfIndex].columnwidths[j];
  j++;
 }while ((gridx >= 0)&&(j<BGHS[SelfIndex].cols));

 if(j>BGHS[SelfIndex].cols){j=BGHS[SelfIndex].cols;}
 BGHS[SelfIndex].rightvisiblecol = j;




 //calc rows visible;
 gridy = gridy - BGHS[SelfIndex].headerrowheight;
 j=	BGHS[SelfIndex].homerow;
 do
 {
  gridy = gridy - BGHS[SelfIndex].rowheight;
  j++;
 }while ((gridy > 0)&&(j<BGHS[SelfIndex].rows));

 if(j>BGHS[SelfIndex].rows){j=BGHS[SelfIndex].rows;}
 BGHS[SelfIndex].bottomvisiblerow = j;
}


RECT GetCellRect(HWND hWnd,int SI, int r, int c)
	{
	 RECT rect;
	 int offset;
	 int j;
	 //c and r must be greater than zero

	 //get column offset
	   //first get col 0 width
	   offset=BGHS[SI].columnwidths[0];
	   for(j=BGHS[SI].homecol;j<c;j++)
		   {
			offset += BGHS[SI].columnwidths[j];
		   }
	   rect.left = offset;
	   rect.right = offset + BGHS[SI].columnwidths[c];

    if(BGHS[SI].EXTENDLASTCOLUMN)
        {
	        //see if this is the last column
	        if(!GetNextColWithWidth(SI,c,1))
		        {
		         //extend this column
		         RECT trect;
		         int temp;
		         GetClientRect(hWnd,&trect);
		         temp = (offset +(trect.right - rect.left))-rect.left;
		         if(temp > BGHS[SI].columnwidths[c])
			         {
		              rect.right = offset + (trect.right - rect.left);
			         }
		        }
        }

	   //now get the top and bottom of the rect
	   offset = BGHS[SI].headerrowheight+BGHS[SI].titleheight;
	   for(j=BGHS[SI].homerow;j<r;j++)
		   {
			offset += BGHS[SI].rowheight;
		   }
	   rect.top = offset;
	   rect.bottom = offset + BGHS[SI].rowheight;
	   return rect;
	}


void DisplayTitle(HWND hWnd,int SI,HFONT hfont)
    {
     RECT rect;
     HDC gdc;
     HFONT holdfont;

     GetClientRect(hWnd,&rect);
     


     gdc=GetDC(hWnd);
     SetBkMode(gdc,TRANSPARENT);
     holdfont=(HFONT)SelectObject(gdc,hfont);
     rect.bottom = BGHS[SI].titleheight;
	 DrawEdge(gdc,&rect,EDGE_ETCHED,BF_MIDDLE|BF_RECT|BF_ADJUST);
	 DrawTextEx(gdc,BGHS[SI].title,-1,&rect,DT_END_ELLIPSIS|DT_CENTER|DT_WORDBREAK|DT_NOPREFIX,NULL);
     SelectObject(gdc,holdfont);
     ReleaseDC(hWnd,gdc);
    }


void DisplayColumn(HWND hWnd,int SI,int c,int offset,HFONT hfont,HFONT hcolumnheadingfont)
{
	HDC gdc;
	RECT rect,rectsave;
    HFONT holdfont;
	int r;
	TCHAR buffer[1000];
	int iDataType,iProtection;
	if(BGHS[SI].columnwidths[c]==0){return;}

	 gdc=GetDC(hWnd);
	 SetBkMode(gdc,TRANSPARENT);
     ShowHscroll(hWnd,SI);
	 ShowVscroll(hWnd,SI);
     

    holdfont = (HFONT)SelectObject(gdc,hcolumnheadingfont);
	SetTextColor(gdc,BGHS[SI].textcolor);
	//display header row
	r=0;



	rect.left = offset + 0;
	rect.top = BGHS[SI].titleheight;//0
	rect.right = BGHS[SI].columnwidths[c] + offset;
	rect.bottom = BGHS[SI].headerrowheight + BGHS[SI].titleheight;

    if(BGHS[SI].EXTENDLASTCOLUMN)
        {
	        //see if this is the last column
	        if(!GetNextColWithWidth(SI,c,1))
		        {
		         //extend this column
		         RECT trect;
		         GetClientRect(hWnd,&trect);
		         
		         rect.right = offset + (trect.right - rect.left);
		        }
        }
    else
        {
         if(!GetNextColWithWidth(SI,c,1))
             {
              //repaint right side of grid
		         RECT trect;
                 HBRUSH holdbrush;
                 HPEN holdpen;
		         GetClientRect(hWnd,&trect);
                 trect.left=offset+(rect.right-rect.left);
                 holdbrush=(HBRUSH)SelectObject(gdc,GetStockObject(GRAY_BRUSH));
                 holdpen=(HPEN)SelectObject(gdc,GetStockObject(NULL_PEN));
                 Rectangle(gdc,trect.left,trect.top+BGHS[SI].titleheight,trect.right+1,trect.bottom+1);
		         SelectObject(gdc,holdbrush);
                 SelectObject(gdc,holdpen);

             }
        }
      
	 SetCell(&BGcell,r,c);
	 lstrcpy(buffer, TEXT(""));
	 SendMessage(hWnd,BGM_GETCELLDATA,(WPARAM)&BGcell,(LPARAM)buffer);
	 if(BGHS[SI].COLUMNSNUMBERED)
	 {
	  if(c>0)
	  {
	   int high,low;
	   high = ((c-1)/26);
	   low = c % 26;
	   if(high == 0){high = 32;}else{high+=64;}
	   if(low == 0){low=26;}
	   low += 64;
	   wsprintf(buffer, TEXT("%c%c"), high,low);
	  }
	 }
	 rectsave=rect;
	 DrawEdge(gdc,&rect,EDGE_ETCHED,BF_MIDDLE|BF_RECT|BF_ADJUST);
	 DrawTextEx(gdc,buffer,-1,&rect,DT_END_ELLIPSIS|DT_CENTER|DT_WORDBREAK|DT_NOPREFIX,NULL);
	 rect=rectsave;

	 r=BGHS[SI].topvisiblerow;
     //set font for grid body
     SelectObject(gdc,hfont);
	 while(r<=BGHS[SI].bottomvisiblerow)
	 {

             //try to set cursor row to different display color
             if((r==BGHS[SI].cursorrow)&&(c>0)&&(BGHS[SI].DRAWHIGHLIGHT))
                 {
                  if(BGHS[SI].GRIDHASFOCUS)
                      {
                       SetTextColor(gdc,BGHS[SI].highlighttextcolor);
                      }
                  else
                      {
                       SetTextColor(gdc,RGB(0,0,0));//set black text for nonfocus grid hilight
                      }
                 }
             else
                 {
                  SetTextColor(gdc,RGB(0,0,0));
                 }

		 rect.top = rect.bottom;
		 rect.bottom = rect.top + BGHS[SI].rowheight;
		 rectsave=rect;
		 SetCell(&BGcell,r,c);
		 lstrcpy(buffer, TEXT(""));
		 SendMessage(hWnd,BGM_GETCELLDATA,(WPARAM)&BGcell,(LPARAM)buffer);
		 if((c==0)&&(BGHS[SI].ROWSNUMBERED))
		 {
		  wsprintf(buffer, TEXT("%d"), r);
		 }
		 if(c==0)
		 {
		  DrawEdge(gdc,&rect,EDGE_ETCHED,BF_MIDDLE|BF_RECT|BF_ADJUST);
		  
		 }
		 else
		 {
		  HBRUSH hbrush,holdbrush;
		  HPEN hpen,holdpen;
		  iProtection=SendMessage(hWnd,BGM_GETPROTECTION,(WPARAM)&BGcell,0);
		  if(BGHS[SI].DRAWHIGHLIGHT)//highlight on
			  {
			   if(r==BGHS[SI].cursorrow)
				   {
				    if(BGHS[SI].GRIDHASFOCUS)
						{
					     hbrush=CreateSolidBrush(BGHS[SI].highlightcolor);
						}
					else
						{
						 hbrush=CreateSolidBrush(RGB(200,200,200));
						}
				   }

			   else
				  {
					  if(iProtection == 1)
					  {
						hbrush=CreateSolidBrush(BGHS[SI].protectcolor);
					  }
					  else
					  {
						hbrush=CreateSolidBrush(BGHS[SI].unprotectcolor);
					  }
				  }
			  }
		  else
			  {
					  if(iProtection == 1)
					  {
						hbrush=CreateSolidBrush(BGHS[SI].protectcolor);
					  }
					  else
					  {
						hbrush=CreateSolidBrush(BGHS[SI].unprotectcolor);
					  }

			  }
		  hpen=CreatePen(PS_SOLID,1,BGHS[SI].gridlinecolor);
		  holdbrush=(HBRUSH)SelectObject(gdc,hbrush);
		  holdpen=(HPEN)SelectObject(gdc,hpen);
		  Rectangle(gdc,rect.left,rect.top,rect.right,rect.bottom);
		  SelectObject(gdc,holdbrush);
		  SelectObject(gdc,holdpen);
		  DeleteObject(hbrush);
		  DeleteObject(hpen);
		 }
		 rect.right -= 2;
		 rect.left += 2;

		 iDataType=SendMessage(hWnd,BGM_GETTYPE,(WPARAM)&BGcell,0);
         if((iDataType < 1)||(iDataType > 5))
             {
              iDataType = 1;//default to alphanumeric data type.. can't happen
             }
		 if(c==0){iDataType = 2;}

		 if(iDataType == 1)//ALPHA
		 {
          if(BGHS[SI].ELLIPSIS)
              {
              DrawTextEx(gdc,buffer,-1,&rect,DT_END_ELLIPSIS|DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX,NULL);
              }
          else
              {
			   DrawTextEx(gdc,buffer,-1,&rect,DT_LEFT|DT_WORDBREAK|DT_EDITCONTROL|DT_NOPREFIX,NULL);
              }
		 }

		 if(iDataType == 2)//NUMERIC
		 {
		  DrawTextEx(gdc,buffer,-1,&rect,DT_END_ELLIPSIS|DT_RIGHT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX,NULL);
		 }

		 if(iDataType == 3)//BOOLEAN TRUE
		 {
		  int k,excess;
		  k=2;
		  rect.top +=k;
		  rect.bottom -=k;
		  rect.left +=0;
		  rect.right -=0;
          if((rect.bottom - rect.top)>24)
              {
               excess=(rect.bottom - rect.top)-16;
               rect.top += (int)(excess/2);
               rect.bottom -= (int)(excess/2);
              }
	      DrawFrameControl(gdc,&rect,DFC_BUTTON,DFCS_BUTTONCHECK|DFCS_CHECKED);
		 }

		 if(iDataType == 4)//BOOLEAN FALSE
		 {
		  int k,excess;
		  k=2;
		  rect.top +=k;
		  rect.bottom -=k;
		  rect.left +=0;
		  rect.right -=0;
          if((rect.bottom - rect.top)>24)
              {
               excess=(rect.bottom - rect.top)-16;
               rect.top += (int)(excess/2);
               rect.bottom -= (int)(excess/2);
              }


	      DrawFrameControl(gdc,&rect,DFC_BUTTON,DFCS_BUTTONCHECK);
		 }

		 if(iDataType == 5) //user drawn graphic
			 {
			   WPARAM wParam;
               buffer[0]=0x20;
               BGHS[SI].ownerdrawitem = generic_atoi(buffer);
							 wParam=MAKEWPARAM((UINT)::GetMenu(hWnd),BGN_OWNERDRAW);
			   SendMessage(GetParent(hWnd),WM_COMMAND,wParam,(LPARAM)&rect);
			 }

         if(BGHS[SI].EDITING)
             {
              DisplayEditString(hWnd, SI, TEXT(""));
             }

         rect=rectsave;
	     r++;
	 }//end while r<=bottomvisiblerow

             {
              //repaint bottom of grid
		         RECT trect;
                 HBRUSH holdbrush;
                 HPEN holdpen;
		         GetClientRect(hWnd,&trect);
                 trect.top = rect.bottom;
                 trect.left = rect.left;
                 trect.right = rect.right;

                 holdbrush=(HBRUSH)SelectObject(gdc,GetStockObject(GRAY_BRUSH));
                 holdpen=(HPEN)SelectObject(gdc,GetStockObject(NULL_PEN));

                 Rectangle(gdc,trect.left,trect.top,trect.right+1,trect.bottom+1);

		         SelectObject(gdc,holdbrush);
                 SelectObject(gdc,holdpen);

             }


     SelectObject(gdc,holdfont);
     DeleteObject(holdfont);
	 ReleaseDC(hWnd,gdc);



}





void DrawCursor(HWND hWnd,int SI)
	{
	   RECT rect,rectwhole;
	   HDC gdc;
	   HPEN hpen,holdpen;
	   int rop;
       if(BGHS[SI].rows == 0){return;}
	   GetClientRect(hWnd,&rect);
	   //if active cell has scrolled off the top, don't draw a focus rectangle
	   if(BGHS[SI].cursorrow < BGHS[SI].homerow){return;}
       //if active cell has scrolled off to the left, don't draw a focus rectangle
       if(BGHS[SI].cursorcol < BGHS[SI].homecol){return;}

	   rect = GetCellRect(hWnd,SI,BGHS[SI].cursorrow,BGHS[SI].cursorcol);
	   rectwhole=rect;
	   gdc=GetDC(hWnd);
       BGHS[SI].activecellrect = rect;
	   rop=GetROP2(gdc);
	   SetROP2(gdc,R2_XORPEN);
	   SelectObject(gdc,(HBRUSH)GetStockObject(NULL_BRUSH));
	   hpen=CreatePen(PS_SOLID,3,BGHS[SI].cursorcolor);  //width of 3
	   holdpen=(HPEN)SelectObject(gdc,hpen);
	   Rectangle(gdc,rect.left,rect.top,rect.right,rect.bottom);
	   SelectObject(gdc,holdpen);
	   DeleteObject(hpen);
	   SetROP2(gdc,rop);
	   ReleaseDC(hWnd,gdc);
	}

void SetCurrentCellStatus(HWND hWnd,int SelfIndex)
    {
       SetCell(&BGcell,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
       if(SendMessage(hWnd,BGM_GETPROTECTION,(WPARAM)&BGcell,0))
           {
            BGHS[SelfIndex].CURRENTCELLPROTECTED = TRUE;
           }
       else
           {
            BGHS[SelfIndex].CURRENTCELLPROTECTED = FALSE;
           }

    }



TCHAR GetASCII(WPARAM wParam, LPARAM lParam)
    {
     int returnvalue;
     TCHAR mbuffer[100];
     int result;
     BYTE keys[256];
     WORD dwReturnedValue;
     GetKeyboardState(keys);
     result=ToAscii(wParam,(lParam >> 16) && 0xff,keys,&dwReturnedValue,0);
     returnvalue = (TCHAR) dwReturnedValue;
     if(returnvalue < 0){returnvalue = 0;}
     wsprintf(mbuffer, TEXT("return value = %d"), returnvalue);
     if(result!=1){returnvalue = 0;}
     return (TCHAR)returnvalue;

    }



void SetHomeRow(HWND hWnd,int SI,int row,int col)
	{
	 RECT gridrect,cellrect;
	 //get rect of grid window
	 GetClientRect(hWnd,&gridrect);
	 //get rect of current cell
	 cellrect=GetCellRect(hWnd,SI,row,col);
	 if((cellrect.bottom > gridrect.bottom)&&((cellrect.bottom - cellrect.top)<(gridrect.bottom-(BGHS[SI].headerrowheight+BGHS[SI].titleheight))))
		 {
			 while(cellrect.bottom > gridrect.bottom)
				 {
				  BGHS[SI].homerow ++;
				  if(row==BGHS[SI].rows)
					  {
					   gridrect.top = gridrect.bottom - (BGHS[SI].rowheight);
				       InvalidateRect(hWnd,&gridrect,TRUE);
					  }
				  else
					  {
					   InvalidateRect(hWnd,&gridrect,FALSE);
					  }
				  cellrect=GetCellRect(hWnd,SI,row,col);
				 }
		 }
     else
         {
          if((cellrect.bottom - cellrect.top)>=(gridrect.bottom - (BGHS[SI].headerrowheight+BGHS[SI].titleheight)))
              {
               BGHS[SI].homerow++;
              }
         }
	 cellrect=GetCellRect(hWnd,SI,row,col);
		 {
			 while((row < BGHS[SI].homerow))
				 {
				  BGHS[SI].homerow --;
				  InvalidateRect(hWnd,&gridrect,FALSE);
				  cellrect=GetCellRect(hWnd,SI,row,col);
				 }
		 }
	 //set the vertical scrollbar position
		 SetScrollPos(hWnd,SB_VERT,BGHS[SI].homerow,TRUE);
	}



void SetHomeCol(HWND hWnd,int SI,int row,int col)
	{
      RECT gridrect,cellrect;
      BOOL LASTCOLVISIBLE;
      //get rect of grid window
      GetClientRect(hWnd,&gridrect);
      //get rect of current cell
      cellrect = GetCellRect(hWnd,SI,row,col);
      //determine if scroll left or right is needed
      while((cellrect.right > gridrect.right)&&(cellrect.left != BGHS[SI].columnwidths[0]))
          {
           //scroll right is needed
           BGHS[SI].homecol++;
           //see if last column is visible
           cellrect = GetCellRect(hWnd,SI,row,BGHS[SI].cols);
           if(cellrect.right <= gridrect.right)
               {
                LASTCOLVISIBLE=TRUE;
               }
           else
               {
                LASTCOLVISIBLE=FALSE;
               }
           cellrect = GetCellRect(hWnd,SI,row,col);
           InvalidateRect(hWnd,&gridrect,FALSE);
          }
      cellrect = GetCellRect(hWnd,SI,row,col);
      while((BGHS[SI].cursorcol < BGHS[SI].homecol)&&(BGHS[SI].homecol > 1))

          {
           //scroll left is needed
           BGHS[SI].homecol--;
           //see if last column is visible
           cellrect = GetCellRect(hWnd,SI,row,BGHS[SI].cols);
           if(cellrect.right <= gridrect.right)
               {
                LASTCOLVISIBLE=TRUE;
               }
           else
               {
                LASTCOLVISIBLE=FALSE;
               }

           cellrect = GetCellRect(hWnd,SI,row,col);
           InvalidateRect(hWnd,&gridrect,FALSE);
          }
          {
          int k;
           k=HomeColumnNthVisible(SI);
           SetScrollPos(hWnd,SB_HORZ,k,TRUE);
          }
	}



void ShowVscroll(HWND hWnd,int SI)
	{
	 //if more rows than can be visible on grid, display vertical scrollbar
	 //otherwise, hide it.
	 RECT gridrect;
	 int totalpixels;
	 int rowsvisibleonscreen;
	 GetClientRect(hWnd,&gridrect);
	 totalpixels = gridrect.bottom;
     totalpixels -= BGHS[SI].titleheight;
	 totalpixels -= BGHS[SI].headerrowheight;
	 totalpixels -= (BGHS[SI].rowheight * BGHS[SI].rows);
	 rowsvisibleonscreen = (gridrect.bottom - (BGHS[SI].headerrowheight+BGHS[SI].titleheight)) / BGHS[SI].rowheight;
	 if(totalpixels < 0)
		 {
		  //show vscrollbar
		 ShowScrollBar(hWnd,SB_VERT,TRUE);
		 SetScrollRange(hWnd,SB_VERT,1,(BGHS[SI].rows-rowsvisibleonscreen)+1,TRUE);
         BGHS[SI].VSCROLL = TRUE;
		 }
	 else
		 {
		  //hide vscrollbar
		  ShowScrollBar(hWnd,SB_VERT,FALSE);
          BGHS[SI].VSCROLL = FALSE;
		 }

	}

void ShowHscroll(HWND hWnd,int SI)
	{
	 //if more rows than can be visible on grid, display vertical scrollbar
	 //otherwise, hide it.
	 RECT gridrect;
	 int totalpixels;
	 int colswithwidth;
     int j;
	 GetClientRect(hWnd,&gridrect);
	 totalpixels = gridrect.right;
	 totalpixels -= BGHS[SI].columnwidths[0];
     colswithwidth = 0;
     for(j=1;j<=BGHS[SI].cols;j++)
         {
          totalpixels -= BGHS[SI].columnwidths[j];
          if(BGHS[SI].columnwidths[j]>0)
              {
               colswithwidth++;
              }
         }
	 if(totalpixels < 0)
		 {
		  //show hscrollbar
		 ShowScrollBar(hWnd,SB_HORZ,TRUE);
		 SetScrollRange(hWnd,SB_HORZ,1,colswithwidth,TRUE);
         BGHS[SI].HSCROLL = TRUE;
		 }
	 else
		 {
		  //hide hscrollbar
		  ShowScrollBar(hWnd,SB_HORZ,FALSE);
          BGHS[SI].HSCROLL = FALSE;
		 }

	}



void NotifyRowChanged(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_ROWCHANGED);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);
	   wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_SELCHANGE);
	   SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);
	}


void NotifyColChanged(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_COLCHANGED);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_SELCHANGE);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);

	}


void NotifyEndEdit(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_EDITEND);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);

	}


void NotifyDelete(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_DELETECELL);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);

	}


void NotifyEditBegin(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_EDITBEGIN);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);

	}

void NotifyEditEnd(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_EDITEND);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);

	}

/*
void NotifyF1(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_F1);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);

	}

void NotifyF2(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_F2);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);

	}

void NotifyF3(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_F3);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);

	}

void NotifyF4(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_F4);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);

	}

void NotifyF5(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_F5);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);

	}

void NotifyF6(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_F6);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);

	}

void NotifyF7(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_F7);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);

	}

void NotifyF8(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_F8);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);

	}

void NotifyF9(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_F9);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);

	}

void NotifyF10(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_F10);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);

	}

void NotifyF11(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_F11);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);

	}

void NotifyF12(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_F12);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);

	}
*/
void NotifyCellClicked(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_CELLCLICKED);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);

	}

void NotifyCellDbClicked(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_CELLDBCLICKED);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);
	}

void NotifyCellRClicked(HWND hWnd,int SI)
	{
	 WPARAM wParam;
	 LPARAM lParam;
	   lParam = MAKELPARAM(BGHS[SI].cursorrow,BGHS[SI].cursorcol);
       wParam=MAKEWPARAM((UINT)BGHS[SI].gridmenu,BGN_CELLRCLICKED);
       SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);
	}
void GetVisibleColumns(HWND hWnd,int SI)
    {
     int j;
     int value;
     value=0;
     for(j=1;j<=BGHS[SI].cols;j++)
         {
          if(BGHS[SI].columnwidths[j] > 0)
              {
               value++;
              }
         }
     BGHS[SI].visiblecolumns = value;
     SetScrollRange(hWnd,SB_HORZ,1,value,TRUE);
    }

int GetNthVisibleColumn(HWND, int SI, int n)
    {
     int j,count;
     int value;
     j=1;
     count=0;
     value = n-1;
     while(j<=BGHS[SI].cols)
         {
          if(BGHS[SI].columnwidths[j]>0)
              {
               count++;
               if(count==n)
                   {
                    value = j;
                   }
              }
          j++;
         }
     return value;
    }


void CloseEdit(HWND hWnd,int SI)
    {
     int r,c;
     _BGCELL cell;
     r=BGHS[SI].cursorrow;
     c=BGHS[SI].cursorcol;
     cell.row = r;
     cell.col = c;
     SendMessage(hWnd,BGM_SETCELLDATA,(WPARAM)&cell,(LPARAM)BGHS[SI].editstring);
     lstrcpy(BGHS[SI].editstring, TEXT(""));
     RefreshGrid(hWnd);
     BGHS[SI].EDITING = FALSE;
     HideCaret(hWnd);
     NotifyEditEnd(hWnd,SI);
    }

void DisplayEditString(HWND hWnd,int SI,TCHAR* tstring)
    {
       int r,c;
       HFONT holdfont;
       RECT rt;
       HDC cdc;
       r=BGHS[SI].cursorrow;
       c=BGHS[SI].cursorcol;
       ShowCaret(hWnd);
       if((r<BGHS[SI].homerow)||(c<BGHS[SI].homecol))
           {
            HideCaret(hWnd);
            return;
           }
       rt=GetCellRect(hWnd,SI,r,c);
       rt.top += 2;
       rt.bottom -= 2;
       rt.right -=2;
       rt.left += 2;

       cdc=GetDC(hWnd);
       Rectangle(cdc,rt.left,rt.top,rt.right,rt.bottom);
       rt.top += 2;
       rt.bottom -= 2;
       rt.right -=2;
       rt.left += 2;

       if(lstrlen(BGHS[SI].editstring)<=300)
           {
            lstrcat(BGHS[SI].editstring,tstring);
            lstrcpy(BGHS[SI].editstringdisplay,BGHS[SI].editstring);
           }
       else
           {
            MessageBeep(0);
           }

       holdfont=(HFONT)SelectObject(cdc,BGHS[SI].hfont);
       rt.right -= 5;
       DrawText(cdc,BGHS[SI].editstringdisplay,-1,&rt,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
       rt.right +=5;
       ShowCaret(hWnd);

           {
            int rh,ah;
            rh=BGHS[SI].rowheight;
            ah=BGHS[SI].fontascentheight;

            SetCaretPos(rt.right-4,rt.top+(int)(rh/2)-ah+2);
       
           }

       SelectObject(cdc,holdfont);
       ReleaseDC(hWnd,cdc);
    }
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////


ATOM RegisterGridClass(HINSTANCE hInstance)
{
    //initialize BGHS structure

   for(int j = 0 ; j < MAX_GRIDS ; j++)
   {
        BGHS[j].gridmenu = 0;
        BGHS[j].hlist1 = NULL; 
		lstrcpy(BGHS[j].protect, TEXT("U"));
		BGHS[j].rows = 100;
		BGHS[j].cols = 255;
		BGHS[j].homerow = 1;
		BGHS[j].homecol = 1;
		BGHS[j].rowheight = 21;
		BGHS[j].headerrowheight = 21;
		BGHS[j].ROWSNUMBERED = TRUE;
		BGHS[j].COLUMNSNUMBERED = TRUE;
        BGHS[j].EDITABLE = FALSE;
        BGHS[j].EDITING = FALSE;
        BGHS[j].AUTOROW = TRUE;
		BGHS[j].cursorcol = 1;  
		BGHS[j].cursorrow = 1;  
		BGHS[j].columnwidths[0]=50;
        BGHS[j].ADVANCEROW = TRUE;
		BGHS[j].DRAWHIGHLIGHT = TRUE;
		BGHS[j].cursorcolor = RGB(255,255,255);
		BGHS[j].protectcolor = RGB(255,255,255); 
		BGHS[j].unprotectcolor = RGB(255,255,255);
		BGHS[j].highlightcolor = RGB(0,0,128); 
		BGHS[j].gridlinecolor = RGB(220,220,220);
        BGHS[j].highlighttextcolor = RGB(255,255,255);
		BGHS[j].textcolor = RGB(0,0,0);
        BGHS[j].titleheight = 0;
        BGHS[j].EXTENDLASTCOLUMN = TRUE;
        BGHS[j].SHOWINTEGRALROWS = TRUE;
        BGHS[j].SIZING = FALSE;
        BGHS[j].ELLIPSIS = TRUE;
        BGHS[j].COLAUTOWIDTH = FALSE;
        BGHS[j].COLUMNSIZING = FALSE;
        BGHS[j].ALLOWCOLUMNRESIZING = FALSE;
        BGHS[j].cursortype = 0;
        BGHS[j].hcolumnheadingfont = NULL;
        BGHS[j].htitlefont = NULL;
        lstrcpy(BGHS[j].editstring, TEXT(""));

		for(int k = 0 ; k < MAX_COLS ; k++)
		{
			BGHS[j].columnwidths[k]=50;
		}
		
    }

	WNDCLASS wclass;

	//wclass.style = CS_BYTEALIGNWINDOW;//CS_HREDRAW|CS_VREDRAW;
    wclass.style = CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS;
	wclass.lpfnWndProc = (WNDPROC)GridProc;
	wclass.cbClsExtra = 0;
	wclass.cbWndExtra = 0;
	wclass.hInstance = hInstance;
	wclass.hIcon = NULL;
	wclass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	
	wclass.hbrBackground = (HBRUSH)(GetStockObject(GRAY_BRUSH));
	wclass.lpszClassName = TEXT("BABYGRID");
	wclass.lpszMenuName = NULL;

	return RegisterClass(&wclass);
}


void SizeGrid(HWND hWnd,int SI)
    {
     SendMessage(hWnd,WM_SIZE,SIZE_MAXIMIZED,MAKELPARAM(BGHS[SI].wannabewidth,BGHS[SI].wannabeheight));
     SendMessage(hWnd,WM_SIZE,SIZE_MAXIMIZED,MAKELPARAM(BGHS[SI].wannabewidth,BGHS[SI].wannabeheight));
     
    }

int FindLongestLine(HDC hdc,TCHAR* text,SIZE* size)
{
	int longest, lines;
     TCHAR temptext[1000];
     TCHAR *p;
     longest=0;
     lines=1;

     for(size_t j=0, len=lstrlen(text); j<len; j++)
     {
      if(text[j]=='\n')
          {
           lines++;
          }
     }
     lstrcpy(temptext,text);
     p = generic_strtok(temptext, TEXT("\n"));
     while(p)
         {
          GetTextExtentPoint32(hdc,p,lstrlen(p),size);
          if(size->cx > longest)
              {
               longest=size->cx;
              }
          p = generic_strtok('\0', TEXT("\n"));
         }
     return longest;
    }


LRESULT CALLBACK GridProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	TCHAR buffer[1000];
	int SelfIndex;
	int ReturnValue;
    UINT SelfMenu;
	HINSTANCE hInst;
    int iDataType;
    static int ASCII;
    

	SelfIndex=FindGrid((UINT)GetMenu(hWnd));
    SelfMenu=BGHS[SelfIndex].gridmenu;

	//update the grid width and height variable
	{
	 RECT rect;
	 
	 GetClientRect(hWnd,&rect);
	 BGHS[SelfIndex].gridwidth = rect.right - rect.left;
	 BGHS[SelfIndex].gridheight = rect.bottom - rect.top;

	}

	ReturnValue = 0;

	switch (message) 
	{
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
                case 1:
				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;

		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			RECT rt;
			GetClientRect(hWnd, &rt);
			CalcVisibleCellBoundaries(SelfIndex);
            //display title
            DisplayTitle(hWnd,SelfIndex,BGHS[SelfIndex].htitlefont);
			//display column 0;

			DisplayColumn(hWnd,SelfIndex,0,0,BGHS[SelfIndex].hfont,BGHS[SelfIndex].hcolumnheadingfont);
			    {
			     int c,j,k,offset;
			     offset = BGHS[SelfIndex].columnwidths[0];
			     j=BGHS[SelfIndex].leftvisiblecol;
			     k=BGHS[SelfIndex].rightvisiblecol;
			     for(c=j;c<=k;c++)
			         {
			          DisplayColumn(hWnd,SelfIndex,c,offset,BGHS[SelfIndex].hfont,BGHS[SelfIndex].hcolumnheadingfont);
			          offset+=BGHS[SelfIndex].columnwidths[c];
			         }
			 
			    }
			EndPaint(hWnd, &ps);
			//
			if(GetFocus()==hWnd)
				{
			      PostMessage(hWnd,BGM_DRAWCURSOR,(UINT)SelfIndex,0);
				}


			break;


        case BGM_PAINTGRID:
            {
             RECT rect;
             GetClientRect(hWnd, &rect);
             InvalidateRect(hWnd,&rect,TRUE);
             UpdateWindow(hWnd);
             MessageBeep(0);
            }
            break;
        case WM_SETTEXT: 
            { 
              int j,linecount;
              SIZE size;
              HDC gdc;
              HFONT holdfont;
              if(lstrlen((TCHAR*)lParam)>300)
                  {
                   lstrcpy(BGHS[SelfIndex].title, TEXT("Title too long (300 chars max)"));
                  }
              else
                  {
                    lstrcpy(BGHS[SelfIndex].title,(TCHAR*)lParam);
                  }
             
             gdc=GetDC(hWnd);
             //get linecount of title;
             if(lstrlen(BGHS[SelfIndex].title) > 0)
                 {
                  linecount=1;
                  for(j=0;j<(int)lstrlen(BGHS[SelfIndex].title);j++)     
                      {
                       if(BGHS[SelfIndex].title[j]=='\n')
                           {
                            linecount++;
                           }

                      }
                  holdfont=(HFONT)SelectObject(gdc,BGHS[SelfIndex].htitlefont);
                  GetTextExtentPoint32(gdc,BGHS[SelfIndex].title,lstrlen(BGHS[SelfIndex].title),&size);
                  SelectObject(gdc,holdfont);
                  BGHS[SelfIndex].titleheight = (int)((size.cy*1.2) * linecount);
                 }
             else
                 {
                  //no title
                  BGHS[SelfIndex].titleheight = 0;
                 }
              ReleaseDC(hWnd,gdc);

              
			  RefreshGrid(hWnd);
              SizeGrid(hWnd,SelfIndex);
              
            }
            break;
        case BGM_GETROWS:
              ReturnValue = BGHS[SelfIndex].rows;
            break;

        case BGM_GETCOLS:
              ReturnValue = BGHS[SelfIndex].cols;
            break;

        case BGM_GETCOLWIDTH:
              ReturnValue = BGHS[SelfIndex].columnwidths[wParam];
            break;

        case BGM_GETROWHEIGHT:
               ReturnValue = BGHS[SelfIndex].rowheight;
            break;

        case BGM_GETHEADERROWHEIGHT:
               ReturnValue = BGHS[SelfIndex].headerrowheight;
            break;

        case BGM_GETOWNERDRAWITEM:
              ReturnValue = BGHS[SelfIndex].ownerdrawitem;
            break;

		case BGM_DRAWCURSOR:
			  DrawCursor(hWnd,wParam);
			break;
        case BGM_SETCURSORPOS:
               DrawCursor(hWnd,SelfIndex);
               if((((int)wParam <= BGHS[SelfIndex].rows)&&((int)wParam > 0))&&
                   (((int)lParam <= BGHS[SelfIndex].cols)&&((int)lParam > 0)))
                   {
                     BGHS[SelfIndex].cursorrow=wParam;
                     BGHS[SelfIndex].cursorcol=lParam;
                   }
               else
                   {
                    DrawCursor(hWnd,SelfIndex);
                    break;
                   }
               SetHomeRow(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
               SetHomeCol(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
               DrawCursor(hWnd,SelfIndex);
               RefreshGrid(hWnd);

            break;
		case BGM_SHOWHILIGHT:
			   BGHS[SelfIndex].DRAWHIGHLIGHT = (BOOL)wParam;
               RefreshGrid(hWnd);
			break;
        case BGM_EXTENDLASTCOLUMN:
               BGHS[SelfIndex].EXTENDLASTCOLUMN = (BOOL)wParam;
               RefreshGrid(hWnd);
            break;

        case BGM_SHOWINTEGRALROWS:
               BGHS[SelfIndex].SHOWINTEGRALROWS = (BOOL)wParam;
               SizeGrid(hWnd,SelfIndex);
               RefreshGrid(hWnd);
            break;

        case BGM_SETCOLAUTOWIDTH:
               BGHS[SelfIndex].COLAUTOWIDTH = (BOOL)wParam;
            break;

        case BGM_SETALLOWCOLRESIZE:
               BGHS[SelfIndex].ALLOWCOLUMNRESIZING = (BOOL)wParam;
            break;

		case BGM_PROTECTCELL:
              LPBGcell=(_BGCELL*)wParam;
              if(OutOfRange(LPBGcell))
                  {
                   wParam=MAKEWPARAM((UINT)GetMenu(hWnd),BGN_OUTOFRANGE);
                   lParam = 0;
                   SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);
				   ReturnValue = -1;
                   break;
                  }
              wsprintf(buffer, TEXT("%05d-%03d"), LPBGcell->row,LPBGcell->col);
              //see if that cell is already loaded
              FindResult = BinarySearchListBox(BGHS[SelfIndex].hlist1,buffer);
              if(FindResult != LB_ERR)
                  {
                   //it was found, get the text, modify text delete it from list, add modified to list
				   SendMessage(BGHS[SelfIndex].hlist1,LB_GETTEXT,FindResult,(LPARAM)buffer);
				   if((BOOL)lParam)
				   {
					buffer[10] = 'P';
				   }
				   else
				   {
					buffer[10] = 'U';
				   }
                   SendMessage(BGHS[SelfIndex].hlist1,LB_DELETESTRING,FindResult,0);
				   SendMessage(BGHS[SelfIndex].hlist1,LB_ADDSTRING,FindResult,(LPARAM)buffer);
                  }
			  else
			  {
			   //protecting or unprotecting a cell that isn't in the list
			   //add it as blank;
				  lstrcat(buffer, TEXT("|"));
				  if((BOOL)lParam)
				  {
				   lstrcat(buffer, TEXT("PA"));
				  }
				  else
				  {
				   lstrcat(buffer, TEXT("UA"));
				  }
				  lstrcat(buffer, TEXT("|"));
				  SendMessage(BGHS[SelfIndex].hlist1,LB_ADDSTRING,FindResult,(LPARAM)buffer);
			  }

			break;
		case BGM_NOTIFYROWCHANGED:
			   NotifyRowChanged(hWnd,SelfIndex);
			break;
		case BGM_NOTIFYCOLCHANGED:
			   NotifyColChanged(hWnd,SelfIndex);
			break;
		case BGM_SETPROTECT:
			  if((BOOL)wParam)
                  {
			       lstrcpy(BGHS[SelfIndex].protect, TEXT("P"));
			      }
			      else      
                  {                
			       lstrcpy(BGHS[SelfIndex].protect, TEXT("U"));
			      }
			break;

		case BGM_AUTOROW:
			  if((BOOL)wParam)
			  {
			   BGHS[SelfIndex].AUTOROW = TRUE;
			  }
			  else
			  {
			   BGHS[SelfIndex].AUTOROW = FALSE;
			  }
			break;
        case BGM_SETEDITABLE:
               if((BOOL)wParam)
                   {
                    BGHS[SelfIndex].EDITABLE = TRUE;
                   }
               else
                   {
                    BGHS[SelfIndex].EDITABLE = FALSE;
                   }
            break;

        case BGM_SETCELLDATA:
              LPBGcell=(_BGCELL*)wParam;
              if(OutOfRange(LPBGcell))
                  {
                   wParam=MAKEWPARAM((UINT)GetMenu(hWnd),BGN_OUTOFRANGE);
                   lParam = 0;
                   SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);
				   ReturnValue = -1;
                   break;
                  }
              wsprintf(buffer, TEXT("%05d-%03d"), LPBGcell->row,LPBGcell->col);
              //see if that cell is already loaded
              FindResult = BinarySearchListBox(BGHS[SelfIndex].hlist1,buffer);
              if(FindResult != LB_ERR)
                  {
                   //it was found, delete it
                   SendMessage(BGHS[SelfIndex].hlist1,LB_DELETESTRING,FindResult,0);
                  }
              //now add it
			  lstrcat(buffer, TEXT("|"));
			  lstrcat(buffer,BGHS[SelfIndex].protect);
              //determine data type (text,numeric, or boolean)(1,2,3)
              //iDataType=DetermineDataType((TCHAR*)lParam);

			  iDataType = 1;
              if(iDataType==1){lstrcat(buffer, TEXT("A"));}
              if(iDataType==2){lstrcat(buffer, TEXT("N"));}
              if(iDataType==3){lstrcat(buffer, TEXT("T"));}
              if(iDataType==4){lstrcat(buffer, TEXT("F"));}
			  if(iDataType==5){lstrcat(buffer, TEXT("G"));}

			  lstrcat(buffer, TEXT("|"));
              lstrcat(buffer, (TCHAR*)lParam);
              FindResult=SendMessage(BGHS[SelfIndex].hlist1,LB_ADDSTRING,0,(LPARAM)buffer);
              
              if(FindResult==LB_ERR)
                  {
                   MessageBeep(0);
                  }
                  {
                   RECT rect;
                   rect=GetCellRect(hWnd,SelfIndex,LPBGcell->row,LPBGcell->col);
                   InvalidateRect(hWnd,&rect,FALSE);
                  }
              //get the last line and adjust grid dimmensions
                  if(BGHS[SelfIndex].AUTOROW)
                  {
                   int j;
                    j=SendMessage(BGHS[SelfIndex].hlist1,LB_GETCOUNT,0,0);
                    if(j>0)
                        {
                         SendMessage(BGHS[SelfIndex].hlist1,LB_GETTEXT,j-1,(LPARAM)buffer);
                         buffer[5]=0x00;
                         j=generic_atoi(buffer);
                         if(j>SendMessage(hWnd,BGM_GETROWS,0,0))
                             {
                              SendMessage(hWnd,BGM_SETGRIDDIM,j,BGHS[SelfIndex].cols);
                             }
                        }
                    else
                        {
                         //no items in the list
                        SendMessage(hWnd,BGM_SETGRIDDIM,j,BGHS[SelfIndex].cols);
                        }
                  }

                  //adjust the column width if COLAUTOWIDTH==TRUE
                  if((BGHS[SelfIndex].COLAUTOWIDTH)||(LPBGcell->row == 0))
                      {
                       HDC hdc;
                       SIZE size;
                       int required_width;
                       int current_width;
                       int required_height = 30;
                       int current_height;
                       int longestline;
                       HFONT holdfont;
                       hdc=GetDC(hWnd);
                       if(LPBGcell->row == 0)
                           {
                             holdfont=(HFONT)SelectObject(hdc,BGHS[SelfIndex].hcolumnheadingfont);
                           }
                           else
                           {
                             holdfont=(HFONT)SelectObject(hdc,BGHS[SelfIndex].hfont);
                           }
                       //if there are \n codes in the generic_string, find the longest line
                       longestline=FindLongestLine(hdc,(TCHAR*)lParam,&size);
                       //GetTextExtentPoint32(hdc,(TCHAR*)lParam,lstrlen((TCHAR*)lParam),&size);
                       required_width = longestline+15;
                       required_height = size.cy;
                       //count lines
                           {
                               int count=1;
                               TCHAR tbuffer[255];
                               lstrcpy(tbuffer,(TCHAR*)lParam);
                               for(int j=0;j<(int)lstrlen(tbuffer);j++)
                                   {
                                   if(tbuffer[j]=='\n'){count++;}
                                   }
                               if((!BGHS[SelfIndex].ELLIPSIS)||(LPBGcell->row == 0))
                                   {
                                    required_height *= count;
                                   }
                               required_height +=5;
                           }
                       SelectObject(hdc,holdfont);
                       ReleaseDC(hWnd,hdc);
                       current_width = BGHS[SelfIndex].columnwidths[LPBGcell->col];
                       if(LPBGcell->row == 0)
                           {
                            current_height = BGHS[SelfIndex].headerrowheight;
                            if(required_height > current_height)
                                {
                                 SendMessage(hWnd,BGM_SETHEADERROWHEIGHT,required_height,0);
                                }
                           }
                       else
                           {
                            current_height = BGHS[SelfIndex].rowheight;
                            if(required_height > current_height)
							{
                                 SendMessage(hWnd, BGM_SETROWHEIGHT, /*required_height*/20, 0);
                            }

                           }
                       if(required_width > current_width)
                           {
                            SendMessage(hWnd,BGM_SETCOLWIDTH,LPBGcell->col,required_width);
                           }
                       ReleaseDC(hWnd,hdc);
                      }

            break;

        case BGM_GETCELLDATA:
              LPBGcell=(_BGCELL*)wParam;
              if(OutOfRange(LPBGcell))
                  {
                   wParam=MAKEWPARAM((UINT)GetMenu(hWnd),BGN_OUTOFRANGE);
                   lParam = 0;
                   SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);
				   ReturnValue = -1;
                   break;
                  }
              wsprintf(buffer, TEXT("%05d-%03d"),LPBGcell->row,LPBGcell->col);
              //see if that cell is already loaded
              FindResult = BinarySearchListBox(BGHS[SelfIndex].hlist1,buffer);
              if(FindResult != LB_ERR)
                  {
                   int j,k,c;
                   TCHAR tbuffer[1000];
                   //it was found, get it
                   SendMessage(BGHS[SelfIndex].hlist1,LB_GETTEXT,FindResult,(long)lParam);
                   lstrcpy(tbuffer,(TCHAR*)lParam);
                   k=lstrlen(tbuffer);
                   c=0;
                   for(j=13;j<k;j++)
                       {
                        buffer[c]=tbuffer[j];
                        c++;
                       }
                   buffer[c]=0x00;
                   lstrcpy((TCHAR*)lParam,buffer);
                  }
			  else
			  {
			   lstrcpy((TCHAR*)lParam, TEXT(""));
			  }
            break;

        case BGM_CLEARGRID:
                SendMessage(BGHS[SelfIndex].hlist1,LB_RESETCONTENT,0,0);
                BGHS[SelfIndex].rows = 0;
                BGHS[SelfIndex].cursorrow = 1;
                BGHS[SelfIndex].homerow = 1;
                BGHS[SelfIndex].homecol = 1;
                    {
                     RECT rect;
                     GetClientRect(hWnd,&rect);
                     InvalidateRect(hWnd,&rect,TRUE);
                    }
            break;

        case BGM_DELETECELL:
              LPBGcell=(_BGCELL*)wParam;
              if(OutOfRange(LPBGcell))
                  {
                   wParam=MAKEWPARAM((UINT)GetMenu(hWnd),BGN_OUTOFRANGE);
                   lParam = 0;
                   SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);
				   ReturnValue = -1;
                   break;
                  }
              wsprintf(buffer, TEXT("%05d-%03d"),LPBGcell->row,LPBGcell->col);
              //see if that cell is already loaded
              FindResult = BinarySearchListBox(BGHS[SelfIndex].hlist1,buffer);
              if(FindResult != LB_ERR)
                  {
                   //it was found, delete it
                   SendMessage(BGHS[SelfIndex].hlist1,LB_DELETESTRING,FindResult,0);
				   NotifyEndEdit(hWnd,SelfIndex);
                  }
            break;
		case BGM_SETGRIDDIM:
			   if((wParam>=0)&&(wParam<=MAX_ROWS))
			   {
				BGHS[SelfIndex].rows = wParam;
			   }
			   else
			   {
                if(wParam<0)
                    {
				     BGHS[SelfIndex].rows = 0;
                    }
                else
                    {
                     BGHS[SelfIndex].rows = MAX_ROWS;
                    }
			   }

			   if((lParam>0)&&(lParam<=MAX_COLS))
			   {
				BGHS[SelfIndex].cols = lParam;
			   }
			   else
			   {
                if(lParam <= 0)
                    {
				     BGHS[SelfIndex].cols = 1;
                    }
                else
                    {
                     BGHS[SelfIndex].cols = MAX_COLS;
                    }
			   }
                   {RECT rect;
                    GetClientRect(hWnd,&rect);
                    InvalidateRect(hWnd,&rect,TRUE);
                   }
               GetVisibleColumns(hWnd,SelfIndex);
			break;


		case BGM_SETCOLWIDTH:
			  if((wParam <= MAX_COLS)&&(wParam >= 0) && (lParam >= 0))
			  {
                RECT rect;
			    BGHS[SelfIndex].columnwidths[wParam] = lParam;
                GetClientRect(hWnd,&rect);
                InvalidateRect(hWnd,&rect,FALSE);
                GetVisibleColumns(hWnd,SelfIndex);
			  }
			break;
		case BGM_SETHEADERROWHEIGHT:
			  if(wParam >= 0)             
			  {
               RECT rect;
			   BGHS[SelfIndex].headerrowheight = wParam;
               SizeGrid(hWnd,SelfIndex);
               GetClientRect(hWnd,&rect);
               InvalidateRect(hWnd,&rect,FALSE); 
			  }
			break;

        case BGM_GETROW:
              ReturnValue = BGHS[SelfIndex].cursorrow;
            break;
        case BGM_GETCOL:
              ReturnValue = BGHS[SelfIndex].cursorcol;
            break;

		case BGM_GETTYPE:
              LPBGcell=(_BGCELL*)wParam;
              if(OutOfRange(LPBGcell))
                  {
                   wParam=MAKEWPARAM((UINT)GetMenu(hWnd),BGN_OUTOFRANGE);
                   lParam = 0;
                   SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);
				   ReturnValue = -1;
                   break;
                  }
              wsprintf(buffer, TEXT("%05d-%03d"),LPBGcell->row,LPBGcell->col);
              //see if that cell is already loaded
              FindResult = BinarySearchListBox(BGHS[SelfIndex].hlist1,buffer);
              if(FindResult != LB_ERR)
                  {
                   //it was found, get it
                   SendMessage(BGHS[SelfIndex].hlist1,LB_GETTEXT,FindResult,(LPARAM)buffer);
				   switch (buffer[11])
				   {
				   case 'A':ReturnValue=1;break;
				   case 'N':ReturnValue=2;break;
				   case 'T':ReturnValue=3;break;
				   case 'F':ReturnValue=4;break;
				   case 'G':ReturnValue=5;break;
				   default: ReturnValue =1;break;
				   }
                  }
			break;
		case BGM_GETPROTECTION:
              LPBGcell=(_BGCELL*)wParam;
              if(OutOfRange(LPBGcell))
                  {
                   wParam=MAKEWPARAM((UINT)GetMenu(hWnd),BGN_OUTOFRANGE);
                   lParam = 0;
                   SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);
				   ReturnValue = -1;
                   break;
                  }
              wsprintf(buffer, TEXT("%05d-%03d"),LPBGcell->row,LPBGcell->col);
              //see if that cell is already loaded
			  ReturnValue = 0;
              FindResult = BinarySearchListBox(BGHS[SelfIndex].hlist1,buffer);
              if(FindResult != LB_ERR)
                  {
                   //it was found, get it
                   SendMessage(BGHS[SelfIndex].hlist1,LB_GETTEXT,FindResult,(LPARAM)buffer);
				   switch (buffer[10])
				   {
				   case 'U':ReturnValue=0;break;
				   case 'P':ReturnValue=1;break;
				   default: ReturnValue =0;break;
				   }
                  }

			break;
        case BGM_SETROWHEIGHT:
			if(wParam <1){wParam=1;}
              BGHS[SelfIndex].rowheight = wParam;
               SetHomeRow(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
               SetHomeCol(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
               SizeGrid(hWnd,SelfIndex);

                  {
                    RECT rect;
                    GetClientRect(hWnd,&rect);
                    InvalidateRect(hWnd,&rect,FALSE);
                  }
            break;

        case BGM_SETTITLEHEIGHT:
            if(wParam<0){wParam =0;}
            BGHS[SelfIndex].titleheight = wParam;
               SetHomeRow(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
               SetHomeCol(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
                  {
                    RECT rect;
                    GetClientRect(hWnd,&rect);
                    InvalidateRect(hWnd,&rect,FALSE);
                  }
            break;
        case BGM_SETGRIDLINECOLOR:
			  DrawCursor(hWnd,SelfIndex);
			  BGHS[SelfIndex].gridlinecolor = (COLORREF)wParam;
			  DrawCursor(hWnd,SelfIndex);
              RefreshGrid(hWnd);
            break;

		case BGM_SETCURSORCOLOR:
			  DrawCursor(hWnd,SelfIndex);
			  BGHS[SelfIndex].cursorcolor = (COLORREF)wParam;
			  DrawCursor(hWnd,SelfIndex);
              RefreshGrid(hWnd);
			break;

        case BGM_SETHILIGHTTEXTCOLOR:
               BGHS[SelfIndex].highlighttextcolor = (COLORREF)wParam;
				  {
				   RECT rect;
				   GetClientRect(hWnd,&rect);
				   InvalidateRect(hWnd,&rect,FALSE);
				  }
            break;

        case BGM_SETHILIGHTCOLOR:
               BGHS[SelfIndex].highlightcolor = (COLORREF)wParam;
				  {
				   RECT rect;
				   GetClientRect(hWnd,&rect);
				   InvalidateRect(hWnd,&rect,FALSE);
				  }
            break;


		case BGM_SETPROTECTCOLOR:
			  BGHS[SelfIndex].protectcolor = (COLORREF)wParam;
				  {
				   RECT rect;
				   GetClientRect(hWnd,&rect);
				   InvalidateRect(hWnd,&rect,FALSE);
				  }
			break;
		case BGM_SETUNPROTECTCOLOR:
			  BGHS[SelfIndex].unprotectcolor = (COLORREF)wParam;
				  {
				   RECT rect;
				   GetClientRect(hWnd,&rect);
				   InvalidateRect(hWnd,&rect,FALSE);
				  }
			break;

        case BGM_SETELLIPSIS:
               BGHS[SelfIndex].ELLIPSIS = (BOOL)wParam;
				  {
				   RECT rect;
				   GetClientRect(hWnd,&rect);
				   InvalidateRect(hWnd,&rect,FALSE);
				  }

            break;


        case BGM_SETTITLEFONT:
               BGHS[SelfIndex].htitlefont = (HFONT)wParam;
				  {
				   RECT rect;
				   GetClientRect(hWnd,&rect);
				   InvalidateRect(hWnd,&rect,FALSE);
				  }

            break;

        case BGM_SETHEADINGFONT:
               BGHS[SelfIndex].hcolumnheadingfont = (HFONT)wParam;
				  {
				   RECT rect;
				   GetClientRect(hWnd,&rect);
				   InvalidateRect(hWnd,&rect,FALSE);
				  }

            break;




        case BGM_SETROWSNUMBERED:
               BGHS[SelfIndex].ROWSNUMBERED = (BOOL)wParam;
				  {
				   RECT rect;
				   GetClientRect(hWnd,&rect);
				   InvalidateRect(hWnd,&rect,FALSE);
				  }

            break;

        case BGM_SETCOLSNUMBERED:
               BGHS[SelfIndex].COLUMNSNUMBERED = (BOOL)wParam;
				  {
				   RECT rect;
				   GetClientRect(hWnd,&rect);
				   InvalidateRect(hWnd,&rect,FALSE);
				  }

            break;

		case WM_ENABLE:
			if(wParam == FALSE)
				{
				 BGHS[SelfIndex].textcolor = RGB(120,120,120);
				}
			else
				{
				 BGHS[SelfIndex].textcolor = RGB(0,0,0);
				} 

        case WM_MOUSEMOVE:     
              int x,y,r,c,t,z;
              x=LOWORD(lParam);
              y=HIWORD(lParam);
              r=GetRowOfMouse(SelfIndex,y);
              c=GetColOfMouse(SelfIndex,x);
              t=GetColOfMouse(SelfIndex,x+10);
              z=GetColOfMouse(SelfIndex,x-10);

              if(BGHS[SelfIndex].COLUMNSIZING)
                  {
                   int dx,nx,cr;
                   dx=x-BGHS[SelfIndex].columntoresizeinitx;
                   nx=BGHS[SelfIndex].columntoresizeinitsize + dx;
                   if(nx<=0){nx=0;}
                   cr=BGHS[SelfIndex].columntoresize;
                   SendMessage(hWnd,BGM_SETCOLWIDTH,cr,nx);

                  }
              if((r==0)&&(c>=-1)&&((t!=c)||(z!=c))&&(!BGHS[SelfIndex].COLUMNSIZING))
                  {
                   if((BGHS[SelfIndex].cursortype != 2)&&(BGHS[SelfIndex].ALLOWCOLUMNRESIZING))
                       {
                        BGHS[SelfIndex].cursortype = 2;
                        SetCursor(LoadCursor(NULL, IDC_SIZEWE));
                       }

                  }
              else
                  {
                   if((BGHS[SelfIndex].cursortype != 1)&&(!BGHS[SelfIndex].COLUMNSIZING))
                       {
                        BGHS[SelfIndex].cursortype = 1;
                        SetCursor(LoadCursor(NULL, IDC_ARROW));
                       }
                  }
            break;

        case WM_LBUTTONUP:
              if(BGHS[SelfIndex].COLUMNSIZING)
                  {
                   BGHS[SelfIndex].COLUMNSIZING = FALSE;
                   SetCursor(LoadCursor(NULL, IDC_ARROW));
                   BGHS[SelfIndex].cursortype = 1;
                   BGHS[SelfIndex].SHOWINTEGRALROWS=BGHS[SelfIndex].REMEMBERINTEGRALROWS;
                   SizeGrid(hWnd,SelfIndex);
                  }
            break;

		case WM_RBUTTONUP :
		case WM_LBUTTONDBLCLK :
		case WM_LBUTTONDOWN:
			{
			 int x,y,r,c;
             //check for column sizing
             if(BGHS[SelfIndex].cursortype == 2)
                 {
                  int c,x,t,z;
                  //start column sizing
                  if(!BGHS[SelfIndex].COLUMNSIZING)
					  {
                       BGHS[SelfIndex].REMEMBERINTEGRALROWS = BGHS[SelfIndex].SHOWINTEGRALROWS;
					  }
				  BGHS[SelfIndex].COLUMNSIZING = TRUE;
                  BGHS[SelfIndex].SHOWINTEGRALROWS = FALSE;
                  x=LOWORD(lParam);
                  BGHS[SelfIndex].columntoresizeinitx=x;
                     t=GetColOfMouse(SelfIndex,x+10);
                     z=GetColOfMouse(SelfIndex,x-10);
                  c=GetColOfMouse(SelfIndex,x);
                    if(t!=c)
                        {
                         //resizing column c
                        BGHS[SelfIndex].columntoresize = c;
                        }
                    if(z!=c)
                        {
                         //resizing hidden column to the left of cursor
                        if(c==-1)
                            {
                             c=SendMessage(hWnd,BGM_GETCOLS,0,0);
                            }
                        else
                            {
                              c-=1;
                            }
                         BGHS[SelfIndex].columntoresize = c;
                        }
                  
                  BGHS[SelfIndex].columntoresizeinitsize = BGHS[SelfIndex].columnwidths[c];
                 }

             if(BGHS[SelfIndex].EDITING)
                 {
                  CloseEdit(hWnd,SelfIndex);
                 }
             else
                 {

                  SetFocus(hWnd);
                 }
             BOOL NRC,NCC;
             NRC=FALSE;
             NCC=FALSE;
             
			 if(GetFocus()==hWnd)
				 {
			      
				  x=LOWORD(lParam);
				  y=HIWORD(lParam);
				  r=GetRowOfMouse(SelfIndex,y);
				  c=GetColOfMouse(SelfIndex,x);
				   DrawCursor(hWnd,SelfIndex);
				   if((r>0)&&(c>0))
					   {
					    if(r != BGHS[SelfIndex].cursorrow)
							{
							 BGHS[SelfIndex].cursorrow = r;
                             NRC=TRUE;
							}
						else
							{
					         BGHS[SelfIndex].cursorrow = r;
							}
						if(c != BGHS[SelfIndex].cursorcol)
							{
							 BGHS[SelfIndex].cursorcol = c;
                             NCC=TRUE;
							}
						else
							{
						     BGHS[SelfIndex].cursorcol = c;
							}
						
					   }
                   if(NRC){NotifyRowChanged(hWnd,SelfIndex);}
                   if(NCC){NotifyColChanged(hWnd,SelfIndex);}
				   
				   DrawCursor(hWnd,SelfIndex);
                   SetCurrentCellStatus(hWnd,SelfIndex);
				   SetHomeRow(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
				   SetHomeCol(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
				   RefreshGrid(hWnd);

					if (message == WM_LBUTTONDOWN)
						NotifyCellClicked(hWnd,SelfIndex);
					else if (message == WM_LBUTTONDBLCLK)
						NotifyCellDbClicked(hWnd,SelfIndex);
					else // (message == WM_RBUTTONUP)
						NotifyCellRClicked(hWnd,SelfIndex);
				 }
			 else
				 {
				  SetFocus(hWnd);
				 }
			}
			break;

        case WM_ERASEBKGND:
            return TRUE;
            break;

        case WM_GETDLGCODE:

              ReturnValue = DLGC_WANTARROWS|DLGC_WANTCHARS|DLGC_DEFPUSHBUTTON;
                 if(wParam == 13)
                     {
					 NotifyCellDbClicked(hWnd, SelfIndex);
                   if(BGHS[SelfIndex].EDITING)
                       {
                        CloseEdit(hWnd,SelfIndex);
                       }
				   DrawCursor(hWnd,SelfIndex);

                   SetCurrentCellStatus(hWnd,SelfIndex);
				   SetHomeRow(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
				   RefreshGrid(hWnd);
                   BGHS[SelfIndex].EDITING = FALSE;
                   break;
                     }

              if(wParam == VK_ESCAPE)
                  {
                   if(BGHS[SelfIndex].EDITING)
                       {
                         BGHS[SelfIndex].EDITING = FALSE;
                         lstrcpy(BGHS[SelfIndex].editstring, TEXT(""));
                         HideCaret(hWnd);
                         RefreshGrid(hWnd);
                         NotifyEditEnd(hWnd,SelfIndex);
                       }
                    else
                        {
                          ReturnValue = 0;
                        }
                    break;
                  }
            break;
		case WM_KEYDOWN:

              if(wParam == VK_ESCAPE)
                  {
                   if(BGHS[SelfIndex].EDITING)
                       {
                         BGHS[SelfIndex].EDITING = FALSE;
                         lstrcpy(BGHS[SelfIndex].editstring, TEXT(""));
                         HideCaret(hWnd);
                         RefreshGrid(hWnd);
                         NotifyEditEnd(hWnd,SelfIndex);
                       }
                   break;
                  }
/*
              if(wParam == VK_F1)
                  {
                   NotifyF1(hWnd,SelfIndex);
                   break;
                  }

              if(wParam == VK_F2)
                  {
                   NotifyF2(hWnd,SelfIndex);
                   break;
                  }
              if(wParam == VK_F3)
                  {
                   NotifyF3(hWnd,SelfIndex);
                   break;
                  }
              if(wParam == VK_F4)
                  {
                   NotifyF4(hWnd,SelfIndex);
                   break;
                  }
              if(wParam == VK_F5)
                  {
                   NotifyF5(hWnd,SelfIndex);
                   break;
                  }
              if(wParam == VK_F6)
                  {
                   NotifyF6(hWnd,SelfIndex);
                   break;
                  }
              if(wParam == VK_F7)
                  {
                   NotifyF7(hWnd,SelfIndex);
                   break;
                  }
              if(wParam == VK_F8)
                  {
                   NotifyF8(hWnd,SelfIndex);
                   break;
                  }
              if(wParam == VK_F9)
                  {
                   NotifyF9(hWnd,SelfIndex);
                   break;
                  }
              if(wParam == VK_F10)
                  {
                   NotifyF10(hWnd,SelfIndex);
                   break;
                  }
              if(wParam == VK_F11)
                  {
                   NotifyF11(hWnd,SelfIndex);
                   break;
                  }
              if(wParam == VK_F12)
                  {
                   NotifyF12(hWnd,SelfIndex);
                   break;
                  }
*/
			  if(wParam == VK_DELETE)
				  {
                   NotifyDelete(hWnd,SelfIndex);
				   break;
				  }
              if(wParam == VK_TAB)
                  {
                   SetFocus(GetParent(hWnd));
                   break;
                  }
			  if(wParam == VK_NEXT)
				  {
				   RECT gridrect;
				   int rpp;
                   if(BGHS[SelfIndex].EDITING)
                       {
                        CloseEdit(hWnd,SelfIndex);
                       }

                   if(BGHS[SelfIndex].rows == 0){break;}
				   if(BGHS[SelfIndex].cursorrow == BGHS[SelfIndex].rows){break;}
				   //get rows per page
				   GetClientRect(hWnd,&gridrect);
				   rpp = (gridrect.bottom - (BGHS[SelfIndex].headerrowheight+BGHS[SelfIndex].titleheight))/BGHS[SelfIndex].rowheight;
				   DrawCursor(hWnd,SelfIndex);
				   BGHS[SelfIndex].cursorrow += rpp;
				   
				   if(BGHS[SelfIndex].cursorrow > BGHS[SelfIndex].rows)
					   {
						BGHS[SelfIndex].cursorrow = BGHS[SelfIndex].rows;
					   }
				   NotifyRowChanged(hWnd,SelfIndex);
				   DrawCursor(hWnd,SelfIndex);
				   SetCurrentCellStatus(hWnd,SelfIndex);
				   SetHomeRow(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
                   SetHomeCol(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
				   RefreshGrid(hWnd);
                   break;

				  }
			  if(wParam == VK_PRIOR)
				  {
				   RECT gridrect;
				   int rpp;
                   if(BGHS[SelfIndex].EDITING)
                       {
                        CloseEdit(hWnd,SelfIndex);
                       }

                   if(BGHS[SelfIndex].rows == 0){break;}
				   if(BGHS[SelfIndex].cursorrow == 1){break;}
				   //get rows per page
				   GetClientRect(hWnd,&gridrect);
				   rpp = (gridrect.bottom - (BGHS[SelfIndex].headerrowheight+BGHS[SelfIndex].titleheight))/BGHS[SelfIndex].rowheight;
				   DrawCursor(hWnd,SelfIndex);
				   BGHS[SelfIndex].cursorrow -= rpp;
				   if(BGHS[SelfIndex].cursorrow < 1)
					   {
						BGHS[SelfIndex].cursorrow = 1;
					   }
				   NotifyRowChanged(hWnd,SelfIndex);
				   DrawCursor(hWnd,SelfIndex);
				   SetCurrentCellStatus(hWnd,SelfIndex);
				   SetHomeRow(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
				   SetHomeCol(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
				   RefreshGrid(hWnd);
                   break;
				  }
			  if(wParam == VK_DOWN)	
				  {
                   if(BGHS[SelfIndex].EDITING)
                       {
                        CloseEdit(hWnd,SelfIndex);
                       }

                   if(BGHS[SelfIndex].rows == 0){break;}
				   if(BGHS[SelfIndex].cursorrow == BGHS[SelfIndex].rows){break;}
				   DrawCursor(hWnd,SelfIndex);
				   BGHS[SelfIndex].cursorrow ++;
				   if(BGHS[SelfIndex].cursorrow > BGHS[SelfIndex].rows)
					   {
					    BGHS[SelfIndex].cursorrow = BGHS[SelfIndex].rows;
					   }
				   else
					   {
				        NotifyRowChanged(hWnd,SelfIndex);
					   }
				   DrawCursor(hWnd,SelfIndex);
                   SetCurrentCellStatus(hWnd,SelfIndex);
				   SetHomeRow(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
				   SetHomeCol(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
				   RefreshGrid(hWnd);
                   break;
				  }
			  if(wParam == VK_UP)
				  {
                   if(BGHS[SelfIndex].EDITING)
                       {
                        CloseEdit(hWnd,SelfIndex);
                       }

                   if(BGHS[SelfIndex].rows == 0){break;}
				   if(BGHS[SelfIndex].cursorrow == 1){break;}

				   DrawCursor(hWnd,SelfIndex);
				   BGHS[SelfIndex].cursorrow --;
				   if(BGHS[SelfIndex].cursorrow < 1)
                       {
                        BGHS[SelfIndex].cursorrow = 1;
                       }
                   else
                       {
				        NotifyRowChanged(hWnd,SelfIndex);
                       }
				   DrawCursor(hWnd,SelfIndex);
                   SetCurrentCellStatus(hWnd,SelfIndex);
				   SetHomeRow(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
				   SetHomeCol(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
				   RefreshGrid(hWnd);
                   break;
				  }

			  if(wParam == VK_LEFT)
				  {
                   int k;
                   if(BGHS[SelfIndex].EDITING)
                       {
                        CloseEdit(hWnd,SelfIndex);
                       }

                   if(!GetNextColWithWidth(SelfIndex,BGHS[SelfIndex].cursorcol,-1))
                       {
                        break;
                       }
				   DrawCursor(hWnd,SelfIndex);
                   k=GetNextColWithWidth(SelfIndex,BGHS[SelfIndex].cursorcol,-1);
                   if(k)
                       {
                        BGHS[SelfIndex].cursorcol = k;
						NotifyColChanged(hWnd,SelfIndex);
                       }
				   DrawCursor(hWnd,SelfIndex);
                   SetCurrentCellStatus(hWnd,SelfIndex);
				   SetHomeRow(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
				   SetHomeCol(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
                   break;
				  }

			  if(wParam == VK_RIGHT)
				  {
                   int k;
                   if(BGHS[SelfIndex].EDITING)
                       {
                        CloseEdit(hWnd,SelfIndex);
                       }
				   DrawCursor(hWnd,SelfIndex);
                   k=GetNextColWithWidth(SelfIndex,BGHS[SelfIndex].cursorcol,1);
                   if(k)
                       {
                        BGHS[SelfIndex].cursorcol = k;
						NotifyColChanged(hWnd,SelfIndex);
                       }
				   DrawCursor(hWnd,SelfIndex);
                   SetCurrentCellStatus(hWnd,SelfIndex);
				   SetHomeRow(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
				   SetHomeCol(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
                   RefreshGrid(hWnd);
                   break;
				  }

              SetCurrentCellStatus(hWnd,SelfIndex);

              

              if((BGHS[SelfIndex].CURRENTCELLPROTECTED)&&(wParam == 13))

                  {
				   DrawCursor(hWnd,SelfIndex);
				   BGHS[SelfIndex].cursorrow ++;
				   if(BGHS[SelfIndex].cursorrow > BGHS[SelfIndex].rows)
					   {
					    BGHS[SelfIndex].cursorrow = BGHS[SelfIndex].rows;
					   }
				   else
					   {
				         NotifyRowChanged(hWnd,SelfIndex);
					   }
				   DrawCursor(hWnd,SelfIndex);
                   SetCurrentCellStatus(hWnd,SelfIndex);
				   SetHomeRow(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
				   RefreshGrid(hWnd);
                   break;
                  }

              if(BGHS[SelfIndex].CURRENTCELLPROTECTED){break;}

              if(!BGHS[SelfIndex].EDITABLE)
                  {
                   int ascii;
                   ascii=GetASCII(wParam,lParam);
                   if(ascii == 13) //enter pressed, treat as arrow down
                       {
                           //same as arrow down
				           DrawCursor(hWnd,SelfIndex);
				           BGHS[SelfIndex].cursorrow ++;
				           if(BGHS[SelfIndex].cursorrow > BGHS[SelfIndex].rows)
							   {
								BGHS[SelfIndex].cursorrow = BGHS[SelfIndex].rows;
							   }
						   else
							   {
							    NotifyRowChanged(hWnd,SelfIndex);
							   }
				           DrawCursor(hWnd,SelfIndex);
                           SetCurrentCellStatus(hWnd,SelfIndex);
						   SetHomeRow(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
						   RefreshGrid(hWnd);
                           break;

                       }

                  }
              
              //if it's not an arrow key, make an edit box in the active cell rectangle
              if((BGHS[SelfIndex].EDITABLE)&&(BGHS[SelfIndex].rows > 0))
                  {
                  
				  SetHomeRow(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
				  DrawCursor(hWnd,SelfIndex);
				  DrawCursor(hWnd,SelfIndex);

                      {
                        int ascii;
                          ascii=GetASCII(wParam,lParam);
                          wParam = ascii;
                          if((wParam >= 32)&&(wParam <= 125))
                              {
                               TCHAR tstring[2];
                               if(!BGHS[SelfIndex].EDITING)
                                   {
                                    NotifyEditBegin(hWnd,SelfIndex);
                                   }
                               BGHS[SelfIndex].EDITING = TRUE;
                               tstring[0]= (TCHAR)wParam;
                               tstring[1]=0x00;
                               DisplayEditString(hWnd,SelfIndex,tstring);
                               break;
                              }
                          if(wParam == 8) //backspace
                              {
                               if(!BGHS[SelfIndex].EDITING)
                                   {
                                    NotifyEditBegin(hWnd,SelfIndex);
                                   }

                              BGHS[SelfIndex].EDITING = TRUE;
                              if(lstrlen(BGHS[SelfIndex].editstring)==0)
                                  {
                                   DisplayEditString(hWnd,SelfIndex, TEXT(""));
                                   break;
                                  }
                              else
                                  {
                                   int j;
                                   j=lstrlen(BGHS[SelfIndex].editstring);
                                   BGHS[SelfIndex].editstring[j-1]=0x00;
                                   DisplayEditString(hWnd,SelfIndex, TEXT(""));
                                  }
                               break;
                              }
                          if(wParam == 13)
                              {
                               //same as arrow down
                               if(BGHS[SelfIndex].EDITING)
                                   {
                                    CloseEdit(hWnd,SelfIndex);
                                   }
				               DrawCursor(hWnd,SelfIndex);
				               BGHS[SelfIndex].cursorrow ++;
				               if(BGHS[SelfIndex].cursorrow > BGHS[SelfIndex].rows)
								   {
								    BGHS[SelfIndex].cursorrow = BGHS[SelfIndex].rows;
								   }
							   else
								   {
							        NotifyRowChanged(hWnd,SelfIndex);
								   }
				               DrawCursor(hWnd,SelfIndex);
                               SetCurrentCellStatus(hWnd,SelfIndex);
							   SetHomeRow(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
							   RefreshGrid(hWnd);
                               BGHS[SelfIndex].EDITING = FALSE;
                               break;
                              }
                      }
                  }
			break;
        case WM_HSCROLL:
                  SetFocus(hWnd);
                  if((LOWORD(wParam==SB_LINERIGHT))||(LOWORD(wParam)==SB_PAGERIGHT))
                      {
                       int cp,np;
                       cp=GetScrollPos(hWnd,SB_HORZ);
                       SetScrollPos(hWnd,SB_HORZ,cp+1,TRUE);
                       cp=GetScrollPos(hWnd,SB_HORZ);
                       np=GetNthVisibleColumn(hWnd,SelfIndex,cp);
                       BGHS[SelfIndex].homecol = np;
                       SetScrollPos(hWnd,SB_HORZ,cp,TRUE);
                       RefreshGrid(hWnd);
                      }  
                  if((LOWORD(wParam==SB_LINELEFT))||(LOWORD(wParam)==SB_PAGELEFT))
                      {
                       int cp,np;
                       cp=GetScrollPos(hWnd,SB_HORZ);
                       SetScrollPos(hWnd,SB_HORZ,cp-1,TRUE);
                       cp=GetScrollPos(hWnd,SB_HORZ);
                       np=GetNthVisibleColumn(hWnd,SelfIndex,cp);
                       BGHS[SelfIndex].homecol = np;
                       SetScrollPos(hWnd,SB_HORZ,cp,TRUE);
                       RefreshGrid(hWnd);
                      }  
                  if(LOWORD(wParam)==SB_THUMBTRACK)
                      {
                       int cp,np;
                       cp=HIWORD(wParam);
                       np=GetNthVisibleColumn(hWnd,SelfIndex,cp);
                       SetScrollPos(hWnd,SB_HORZ,np,TRUE);
                       BGHS[SelfIndex].homecol = np;
                       SetScrollPos(hWnd,SB_HORZ,cp,TRUE);
                       RefreshGrid(hWnd);
                      }
                  

            break;

		case WM_MOUSEWHEEL :
		{
			short zDelta = (short) HIWORD(wParam);
			::SendMessage(hWnd, WM_VSCROLL, zDelta < 0?SB_LINEDOWN:SB_LINEUP, 0);
            return TRUE;
		}

		case WM_VSCROLL:
                  SetFocus(hWnd);
				  if(LOWORD(wParam)==SB_THUMBTRACK)
					  {
					   RECT gridrect;
					   int min,max;
					   BGHS[SelfIndex].homerow = HIWORD(wParam);
					   SetScrollPos(hWnd,SB_VERT,HIWORD(wParam),TRUE);
					   GetClientRect(hWnd,&gridrect);
					   GetScrollRange(hWnd,SB_VERT,&min,&max);
					   if(HIWORD(wParam)==max)
						   {
						    gridrect.top = gridrect.bottom - (BGHS[SelfIndex].rowheight);
							InvalidateRect(hWnd,&gridrect,TRUE);
						   }
					   else
						   {
					        InvalidateRect(hWnd,&gridrect,FALSE);
						   }
					  }

				  if(LOWORD(wParam)==SB_PAGEDOWN)
					  {
					   RECT gridrect;
					   int min,max,sp,rpp;
					   //get rows per page
					   GetClientRect(hWnd,&gridrect);
					   rpp = (gridrect.bottom - (BGHS[SelfIndex].headerrowheight+BGHS[SelfIndex].titleheight))/BGHS[SelfIndex].rowheight;
					   GetScrollRange(hWnd,SB_VERT,&min,&max);
					   sp=GetScrollPos(hWnd,SB_VERT);
					   sp += rpp;
					   if(sp > max){sp=max;}
					   BGHS[SelfIndex].homerow = sp;
					   SetScrollPos(hWnd,SB_VERT,sp,TRUE);
					   SetHomeRow(hWnd,SelfIndex,sp,BGHS[SelfIndex].homecol);
					   if(sp==max)
						   {
						    gridrect.top = gridrect.bottom - (BGHS[SelfIndex].rowheight);
							InvalidateRect(hWnd,&gridrect,TRUE);
						   }
					   else
						   {
					        InvalidateRect(hWnd,&gridrect,FALSE);
						   }

					  }
				  if(LOWORD(wParam)==SB_LINEDOWN)
					  {
					   RECT gridrect;
					   int min,max,sp;
					   //get rows per page
					   GetClientRect(hWnd,&gridrect);
					   GetScrollRange(hWnd,SB_VERT,&min,&max);
					   sp=GetScrollPos(hWnd,SB_VERT);
					   sp += 1;
					   if(sp > max){sp=max;}
					   BGHS[SelfIndex].homerow = sp;
					   SetScrollPos(hWnd,SB_VERT,sp,TRUE);
					   SetHomeRow(hWnd,SelfIndex,sp,BGHS[SelfIndex].homecol);
					   if(sp==max)
						   {
						    gridrect.top = gridrect.bottom - (BGHS[SelfIndex].rowheight);
							InvalidateRect(hWnd,&gridrect,TRUE);
						   }
					   else
						   {
					        InvalidateRect(hWnd,&gridrect,FALSE);
						   }

					  }



				  	if(LOWORD(wParam)==SB_PAGEUP)
					  {
					   RECT gridrect;
					   int min,max,sp,rpp;
					   //get rows per page
					   GetClientRect(hWnd,&gridrect);
					   rpp = (gridrect.bottom - (BGHS[SelfIndex].headerrowheight+BGHS[SelfIndex].titleheight))/BGHS[SelfIndex].rowheight;
					   GetScrollRange(hWnd,SB_VERT,&min,&max);
					   sp=GetScrollPos(hWnd,SB_VERT);
					   sp -= rpp;
					   if(sp < 1){sp=1;}
					   BGHS[SelfIndex].homerow = sp;
					   SetScrollPos(hWnd,SB_VERT,sp,TRUE);
					   SetHomeRow(hWnd,SelfIndex,sp,BGHS[SelfIndex].homecol);
					   if(sp==max)
						   {
						    gridrect.top = gridrect.bottom - (BGHS[SelfIndex].rowheight);
							InvalidateRect(hWnd,&gridrect,TRUE);
						   }
					   else
						   {
					        InvalidateRect(hWnd,&gridrect,FALSE);
						   }

					  }
				  	if(LOWORD(wParam)==SB_LINEUP)
					  {
					   RECT gridrect;
					   int min,max,sp;
					   //get rows per page
					   GetClientRect(hWnd,&gridrect);
					   sp=GetScrollPos(hWnd,SB_VERT);
					   GetScrollRange(hWnd,SB_VERT,&min,&max);
					   sp -= 1;
					   if(sp < 1){sp=1;}
					   BGHS[SelfIndex].homerow = sp;
					   SetScrollPos(hWnd,SB_VERT,sp,TRUE);
					   SetHomeRow(hWnd,SelfIndex,sp,BGHS[SelfIndex].homecol);
					   if(sp==max)
						   {
						    gridrect.top = gridrect.bottom - (BGHS[SelfIndex].rowheight);
							InvalidateRect(hWnd,&gridrect,TRUE);
						   }
					   else
						   {
					        InvalidateRect(hWnd,&gridrect,FALSE);
						   }

					  }
                      RefreshGrid(hWnd);

			break;
		case WM_DESTROY:
			{ int k;
                if(CountGrids()==0)
                    {
                     DeleteObject(hfontbody);
                     DeleteObject(hfontheader);
                     DeleteObject(hfonttitle);
                    }
                SendMessage(BGHS[SelfIndex].hlist1,LB_RESETCONTENT,0,0);
			    DestroyWindow(BGHS[SelfIndex].hlist1);
				BGHS[SelfIndex].gridmenu = 0;
				BGHS[SelfIndex].hlist1 = NULL;
				BGHS[SelfIndex].hfont = NULL;
				lstrcpy(BGHS[SelfIndex].protect, TEXT("U"));
				BGHS[SelfIndex].rows = 100;
				BGHS[SelfIndex].cols = 255;
				BGHS[SelfIndex].homerow = 1;
				BGHS[SelfIndex].homecol = 1;
				BGHS[SelfIndex].rowheight = 20;
				BGHS[SelfIndex].headerrowheight = 20;
				BGHS[SelfIndex].ROWSNUMBERED = TRUE;
				BGHS[SelfIndex].COLUMNSNUMBERED = TRUE;
                BGHS[SelfIndex].DRAWHIGHLIGHT = TRUE;                

				BGHS[SelfIndex].cursorcol = 1;
				BGHS[SelfIndex].cursorrow = 1;
				BGHS[SelfIndex].columnwidths[0]=40;
				BGHS[SelfIndex].ADVANCEROW = TRUE;
				BGHS[SelfIndex].cursorcolor = RGB(255,255,255);
				BGHS[SelfIndex].protectcolor = RGB(128,128,128);
				BGHS[SelfIndex].unprotectcolor = RGB(255,255,255);
				for(k=1;k<MAX_COLS;k++)
					{
					 BGHS[SelfIndex].columnwidths[k]=50;
					}
				}
			break;
		case WM_SETFOCUS:
			   DrawCursor(hWnd,SelfIndex);
			   BGHS[SelfIndex].GRIDHASFOCUS	= TRUE; 
               DrawCursor(hWnd,SelfIndex);
               SetCurrentCellStatus(hWnd,SelfIndex);
               SetHomeRow(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);
               SetHomeCol(hWnd,SelfIndex,BGHS[SelfIndex].cursorrow,BGHS[SelfIndex].cursorcol);

               wParam=MAKEWPARAM((UINT)GetMenu(hWnd),BGN_GOTFOCUS);
               lParam = 0;
               SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);
                   {TEXTMETRIC tm;
                    HDC hdc;
                    hdc=GetDC(hWnd);
                    GetTextMetrics(hdc,&tm);
                    ReleaseDC(hWnd,hdc);
                    BGHS[SelfIndex].fontascentheight = (int)tm.tmAscent;
                    CreateCaret(hWnd,NULL,3,tm.tmAscent);
                   }
			   RefreshGrid(hWnd);
			break;
		case WM_KILLFOCUS:	
               DestroyCaret();
			   DrawCursor(hWnd,SelfIndex);
			   BGHS[SelfIndex].GRIDHASFOCUS	= FALSE;

               wParam=MAKEWPARAM((UINT)GetMenu(hWnd),BGN_LOSTFOCUS);
               lParam = 0;
               SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);
               RefreshGrid(hWnd);

			   
			break;
        case WM_SETFONT:
              BGHS[SelfIndex].hfont = (HFONT)wParam;
              if(!BGHS[SelfIndex].hcolumnheadingfont)
                  {
                   BGHS[SelfIndex].hcolumnheadingfont = (HFONT)wParam;
                  }
              if(!BGHS[SelfIndex].htitlefont)
                  {
                   BGHS[SelfIndex].htitlefont = (HFONT)wParam;
                  }
              RefreshGrid(hWnd);
            break;
        case WM_SIZE:
            {
             static int SI,cheight;
             static int savewidth,saveheight;
             int intin,intout;
             SI=SelfIndex;





             if(BGHS[SI].SIZING)
                 {
                  BGHS[SI].SIZING = FALSE;
                  break;
                 }
                  ShowHscroll(hWnd,SI);
                  ShowVscroll(hWnd,SI);

             if((BGHS[SI].SHOWINTEGRALROWS)&&(BGHS[SI].VSCROLL))
                 {
                  saveheight=HIWORD(lParam);
                  intin=saveheight;
                  savewidth=LOWORD(lParam);
                  cheight=HIWORD(lParam);
                  cheight-=BGHS[SI].titleheight;
                  cheight-=BGHS[SI].headerrowheight;

                      {
                       int sbheight;
                       sbheight=GetSystemMetrics(SM_CYHSCROLL);
                       if(BGHS[SI].HSCROLL)
                           {
                            cheight-=sbheight;
                           }
                       if(BGHS[SI].VSCROLL)
                           {
                            RECT grect,prect;
                            GetClientRect(hWnd,&grect);
                            GetClientRect(GetParent(hWnd),&prect);
                            if((grect.right+sbheight) < prect.right)
                                {
                                 savewidth+=sbheight;
                                }
                           }

                      }



                  if(cheight <= BGHS[SI].rowheight)
                      {
                       break;
                      }
                  else
                      {
                       //calculate fractional part of cheight/rowheight
                       int remainder,nrows;
                       nrows=(int)(cheight/BGHS[SI].rowheight);
                       remainder=cheight-(nrows * BGHS[SI].rowheight);
                       //make the window remainder pixels shorter
                       saveheight -= remainder;
                       saveheight +=4; //+=4
                       intout=saveheight;
                       WINDOWPLACEMENT wp;
                       RECT crect;
                       wp.length = sizeof(wp);
                       GetWindowPlacement(hWnd,&wp);
                       crect=wp.rcNormalPosition;
                       crect.bottom=intout;
					   //crect.bottom = NppParameters::getInstance()->_dpiManager.ScaleY(intout);
                       crect.right=savewidth;
                       BGHS[SI].SIZING = TRUE;

                       BGHS[SI].wannabeheight = HIWORD(lParam);
                       BGHS[SI].wannabewidth = LOWORD(lParam);
					   //NppParameters::getInstance()->_dpiManager.ScaleRect(&crect);
                       MoveWindow(hWnd,crect.left,crect.top,crect.right,crect.bottom,TRUE);
                      }
                 }

            }
            break;
		case WM_CREATE:
			  lpcs = &cs;
			  lpcs = (LPCREATESTRUCT)lParam;
			  
			  hInst = lpcs->hInstance;

			  
              BG_GridIndex = AddGrid((UINT)GetMenu(hWnd));

              if(CountGrids()==1)
                  {
                   hfontbody=CreateFont(16,0,0, 0,
										100,
										FALSE,
										FALSE,FALSE,DEFAULT_CHARSET,
										OUT_DEFAULT_PRECIS,
										CLIP_DEFAULT_PRECIS,
										0,
										0 ,
										TEXT("MS Shell Dlg"));
                   hfontheader=CreateFont(18,0,0, 0,FW_HEAVY,FALSE,FALSE,FALSE,DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 0, 0, TEXT("MS Shell Dlg"));
                   hfonttitle=CreateFont(20,0,0, 0,FW_HEAVY,FALSE,FALSE,FALSE,DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 0, 0, TEXT("MS Shell Dlg"));
                  }


              if((BG_GridIndex >= 0)&&(BG_GridIndex < MAX_GRIDS))//if you aren't over the MAX_GRIDS limit, add a grid
                  {

			          BGHS[BG_GridIndex].gridmenu = (UINT)GetMenu(hWnd);

			          BGHS[BG_GridIndex].hlist1=CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("LISTBOX"), TEXT(""),
				          WS_CHILD|LBS_STANDARD,50,150,200,100,hWnd,NULL,hInst,NULL);

		              BGHS[BG_GridIndex].hfont = hfontbody;
                      BGHS[BG_GridIndex].htitlefont = hfonttitle;
                      BGHS[BG_GridIndex].hcolumnheadingfont = hfontheader;
					  lstrcpy(BGHS[BG_GridIndex].title,lpcs->lpszName);
                      SendMessage(hWnd,WM_SETTEXT,0,(LPARAM)lpcs->lpszName);


                  }
              if(BG_GridIndex == -1)
                  {
                   DestroyWindow(hWnd);
                  }
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return ReturnValue;
}

int CountGrids()
	{
	 int j,count;
	 count=0;
	 for(j=0;j<MAX_GRIDS;j++)
		 if(BGHS[j].gridmenu != 0)
			 {
			  count++;
			 }
	 return count;
	}


int AddGrid( UINT menuid)
    {
     //if grid doesn't exist, add it.  otherwise return existing index + MAX_GRIDS
     //if trying to add more than MAX_GRIDS, return -1;
     int empty_space = -1;
     int returnvalue = 0;
     int j;
     BOOL MATCH;
     MATCH=FALSE;
     for(j=0;j<MAX_GRIDS;j++)
         {
          if(BGHS[j].gridmenu == menuid)
              {
               MATCH=TRUE;
               returnvalue=j;
              }
          if(BGHS[j].gridmenu == 0)
              {
               empty_space = j;
              }
         }

     if((!MATCH)&&(empty_space >= 0))
         {
          BGHS[empty_space].gridmenu = menuid;
          returnvalue=empty_space;
         }
     if(MATCH)
         {
          return returnvalue+MAX_GRIDS;
         }
     if((!MATCH)&&(empty_space == -1))
         {
          return -1;
         }
     return returnvalue;

    }

int FindGrid( UINT menuid)
    {
     //if grid doesn't exist, return -1, else return gridindex
     int returnvalue;
     int j;
     returnvalue = -1;
     for(j=0;j<MAX_GRIDS;j++)
         {
          if(BGHS[j].gridmenu == menuid)
              {
               returnvalue=j;
              }
         }


     return returnvalue;
    }



int BinarySearchListBox(HWND lbhWnd,TCHAR* searchtext)
    {
      int ReturnValue;
      int lbcount;
      int head,tail,finger;
      int FindResult;
      TCHAR tbuffer[1000];
	  TCHAR headtext[1000];
	  TCHAR tailtext[1000];
      int p;
     BOOL FOUND;

     FOUND=FALSE;
     //get count of items in listbox
     lbcount = SendMessage(lbhWnd,LB_GETCOUNT,0,0);
     if(lbcount == 0)
         {
           ReturnValue = LB_ERR;
           return ReturnValue;
         }
     if(lbcount < 12)
         {
          //not worth doing binary search, do regular search
          FindResult = SendMessage(lbhWnd,LB_FINDSTRING,(unsigned int)-1,(LPARAM) searchtext);
          ReturnValue = FindResult;
          return ReturnValue;
         }

     // do a binary search
     head = 0;
     tail = lbcount - 1;

     //is it the head?
     SendMessage(lbhWnd,LB_GETTEXT,head,(LPARAM)headtext);
     headtext[9] = 0x00;

	 p=lstrcmp(searchtext,headtext);
     if(p==0)
         {
          //it was the head
          ReturnValue = head;
          return ReturnValue;
         }
	 if(p<0)
		 {
		  //it was less than the head... not found
		  ReturnValue = LB_ERR;
		  return ReturnValue;
		 }

	 

     //is it the tail?
     SendMessage(lbhWnd,LB_GETTEXT,tail,(LPARAM)tailtext);
     tailtext[9] = 0x00;
	 p=lstrcmp(searchtext,tailtext);
     if(p==0)
         {
          //it was the tail
          ReturnValue = tail;
          return ReturnValue;
         }
	 if(p>0)
		 {
		  //it was greater than the tail... not found
		  ReturnValue = LB_ERR;
		  return ReturnValue;
		 }

     //is it the finger?
     ReturnValue = LB_ERR; 
	 FOUND=FALSE;
	 

     while((!FOUND)&&((tail-head)>1))
         {
                  finger = head + ((tail - head) / 2);

                 SendMessage(lbhWnd,LB_GETTEXT,finger,(LPARAM)tbuffer);
                 tbuffer[9] = 0x00;
                 p=lstrcmp(tbuffer,searchtext);
                 if(p==0)
                     {
                      FOUND=TRUE;
                      ReturnValue = finger;
                     }
         
                 if(p<0)
                     {
                      //change  tail to finger
                      head = finger;
                     }
                 if(p>0)    
                         {
                          //change head to finger
                         tail = finger;
                         }


         }
     return ReturnValue;
    }
    
