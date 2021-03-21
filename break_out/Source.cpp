#include "header.h"
#pragma comment(lib, "winmm.lib")
#define CLIENT_HEIGHT 500
#define CLIENT_WIDTH 300
#define MAX_LIFE 3
#define MAX_BLOCK 15
#define INIT_BAR_LEFT 100
#define INIT_BAR_RIGHT 200
#define INIT_BAR_TOP 400
#define INIT_BAR_BOTTOM 410
#define BAR_SPD 10
#define STATE_STARTMENU 0
#define STATE_INGAME 1
#define STATE_GAMECLEAR 2
#define STATE_GAMEOVER 3
#define STATE_OPTION 4
const int STARTMENU_NUM = 3;
const int GAMECLEARMENU_NUM = 3;
const int GAMEOVERMENU_NUM = 3;
const int OPTIONMENU_NUM = 3;
const int FPS = 60;
const double ASPECT_RATIO = (double)CLIENT_WIDTH/CLIENT_HEIGHT; // width / height
const double EPSILON = 0.01;

static TCHAR szWindowClass[] = "DesktopApp";
static TCHAR szTitle[] = "BREAK OUT";
HWND hWnd;
HINSTANCE hInst;
HBRUSH hb, hwhiteb, hbbg;
HPEN hp, hwhitep;
HDC mhdc;
HBITMAP hBitmap;
MCI_OPEN_PARMS open_bgm;
MCI_PLAY_PARMS play_bgm;
MCI_STATUS_PARMS status_bgm;
HFONT font_title, font_subtitle, font_main;
SIZE size_font_title, size_font_subtitle, size_font_main;
const COLORREF block_colors[3] = {RGB(255, 0, 0), RGB(255, 217, 0), RGB(0, 255, 0)};
int hWndSize = CLIENT_HEIGHT;
int wWndSize = CLIENT_WIDTH;
int block_num = 15;
int point_num = 0;
int point_max = 0;
int bar_left, bar_right, bar_top, bar_bottom;
int flag = STATE_STARTMENU;
int bar_vec = 0;
int bgm_volume = 50;
int se_volume = 50;
int now_selected = 0;


class Block{
private:
	int left;
	int right;
	int top;
	int bottom;
	int life;
public:
	Block(int center_x, int center_y){ left = center_x-25; right = center_x+25; top = center_y-10; bottom = center_y+10; life = MAX_LIFE; }
	int get_left(){ return left; }
	int get_right(){ return right; }
	int get_top(){ return top; }
	int get_bottom(){ return bottom; }
	int get_life(){ return life; }
	COLORREF get_color(int life){ return block_colors[life-1]; }
	void hit();
};

class Point{
private:
	int x;
	int y;
	int spd_x;
	int spd_y;
	int radius;
	bool save;
public:
	Point(int _x, int _y, int _spd_x, int _spd_y){ x = _x; y = _y; spd_x = _spd_x; spd_y = _spd_y; radius = 5; save = true; point_num++; point_max++; }
	int get_x(){ return x; }
	int get_y(){ return y; }
	int get_spd_x(){ return spd_x; }
	int get_spd_y(){ return spd_y; }
	int get_radius(){ return radius; }
	void move(int _x, int _y){ x += _x; y += _y; }
	void reflect_x(){ spd_x = -spd_x; }
	void reflect_y(){ spd_y = -spd_y; }
	bool isSave(){ return save; }
	bool check_drop(){
		if(y > CLIENT_HEIGHT+radius){
			save = false;
			point_num--;
			return true;
		}
		return false;
	}
};

class SE{
private:
	MCI_OPEN_PARMS open_parms;
	MCI_PLAY_PARMS play_parms;
	MCI_DGV_SETAUDIO_PARMS set_se_volume;
	LPCSTR resource;
	bool isloaded;
public:
	SE(LPCSTR _resource){ resource = _resource; isloaded = false; }
	bool isLoaded(){ return isloaded; }
	void load(){
		open_parms.lpstrDeviceType = TEXT("MPEGVideo");
		open_parms.lpstrElementName = resource;
		if(mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, (DWORD_PTR)&open_parms)){
			MessageBox(NULL, TEXT("open mp3 file failed!"), TEXT("Windows Desktop Guided Tour"), NULL);
			PostQuitMessage(0);
		}
		play_parms.dwCallback = (DWORD)hWnd;
		set_se_volume.dwItem = MCI_DGV_SETAUDIO_VOLUME;
		set_se_volume.dwValue = se_volume*5;
		if(mciSendCommand(open_parms.wDeviceID, MCI_SETAUDIO, MCI_DGV_SETAUDIO_VALUE | MCI_DGV_SETAUDIO_ITEM, (DWORD_PTR)&set_se_volume)){
			MessageBox(NULL, TEXT("change volume of se failed!"), TEXT("Windows Desktop Guided Tour"), NULL);
			PostQuitMessage(0);
		}
		isloaded = true;
	}
	void unload(){
		mciSendCommand(open_parms.wDeviceID, MCI_CLOSE, 0, 0);
		isloaded = false;
	}
	void change_volume(){
		set_se_volume.dwValue = se_volume*5;
		if(mciSendCommand(open_parms.wDeviceID, MCI_SETAUDIO, MCI_DGV_SETAUDIO_VALUE | MCI_DGV_SETAUDIO_ITEM, (DWORD_PTR)&set_se_volume)){
			MessageBox(NULL, TEXT("change volume of se failed!"), TEXT("Windows Desktop Guided Tour"), NULL);
			PostQuitMessage(0);
		}
	}
	void play(){
		mciSendCommand(open_parms.wDeviceID, MCI_SEEK, MCI_SEEK_TO_START, (DWORD_PTR)&play_parms);
		mciSendCommand(open_parms.wDeviceID, MCI_PLAY, 0, (DWORD_PTR)&play_parms);
	}
};

SE se_select((LPCSTR)TEXT("resource/se/select_se.wav"));
SE se_reflect((LPCSTR)TEXT("resource/se/reflect_se.wav"));
SE se_break((LPCSTR)TEXT("resource/se/block_break_se.wav"));

std::vector<Point> points;
std::vector<Block> blocks;


void Block::hit(){
	life--;
	if(life<=0){
		block_num--;
		std::random_device rnd;
		std::mt19937 mt(rnd());
		std::uniform_int_distribution<> rand(1, 10);
		points.push_back(Point((left+right)/2, (top+bottom)/2, rand(mt), rand(mt)));
		se_break.play();
	}
}

inline double dot(double px, double py, double qx, double qy){ return px*qx+py*qy; }
inline double cross(double px, double py, double qx, double qy){ return px*qy-py*qx; }
inline double vec_abs(double px, double py, double qx, double qy){ return std::abs(sqrt((qx-px)*(qx-px)+(qy-py)*(qy-py))); }

void StartGame(){
	point_num = 0;
	point_max = 0;
	block_num = MAX_BLOCK;
	points.push_back(Point(200, 300, -3, 3));
	for(int i=0; i<MAX_BLOCK; i++){
		blocks.push_back(Block(50*(i%5+1), 20*(i/5+1)));
	}
	bar_left = INIT_BAR_LEFT;
	bar_right = INIT_BAR_RIGHT;
	bar_top = INIT_BAR_TOP;
	bar_bottom = INIT_BAR_BOTTOM;
	hwhiteb = CreateSolidBrush(RGB(255, 255, 255));
	hwhitep = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
	hp = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	flag = STATE_INGAME;
	now_selected = 0;
	mciSendCommand(open_bgm.wDeviceID, MCI_CLOSE, 0, 0);
	se_select.unload();
	se_reflect.load();
	se_break.load();
}

void EndGame(){
	se_reflect.unload();
	se_break.unload();
	se_select.load();
}

void DeleteObj(){
	DeleteObject(hb);
	DeleteObject(hwhiteb);
	DeleteObject(hp);
	DeleteObject(hwhitep);
	mciSendCommand(open_bgm.wDeviceID, MCI_CLOSE, 0, 0);
	se_select.unload();
	se_reflect.unload();
	se_break.unload();
	int psize = points.size();
	int bsize = blocks.size();
	for(int i=0; i<psize; i++){
		points.pop_back();
	}
	for(int i=0; i<bsize; i++){
		blocks.pop_back();
	}
	point_num = 0;
	block_num = 0;
}

double CheckCross(int point_th, double point_delta_x, double point_delta_y, int block_x_1, int block_y_1, int block_x_2, int block_y_2){
	//pass block position with processing about point radius
	//if cross return ratio(point/delta) between start position of point and crossing point
	//else return 2
	int point_x = points[point_th].get_x();
	int point_y = points[point_th].get_y();
	if(cross(point_delta_x, point_delta_y, block_x_1-point_x, block_y_1-point_y)*cross(point_delta_x, point_delta_y, block_x_2-point_x, block_y_2-point_y)<0 &&
		cross(block_x_2-block_x_1, block_y_2-block_y_1, point_x-block_x_1, point_y-block_y_1)*cross(block_x_2-block_x_1, block_y_2-block_y_1, point_x+point_delta_x-block_x_1, point_y+point_delta_y-block_y_1)<0){
		double part = std::abs(cross(point_x-block_x_1, point_y-block_y_1, block_x_2-block_x_1, block_y_2-block_y_1));
		double whole = std::abs(cross(point_x-block_x_1, point_y-block_y_1, block_x_2-block_x_1, block_y_2-block_y_1))+std::abs(cross(point_x+point_delta_x-block_x_1, point_y+point_delta_y-block_y_1, block_x_2-block_x_1, block_y_2-block_y_1));
		return part/whole;
	} else{
		if(vec_abs(point_x+point_delta_x, point_y+point_delta_y, block_x_1, block_y_1)+vec_abs(point_x+point_delta_x, point_y+point_delta_y, block_x_2, block_y_2) == vec_abs(block_x_1, block_y_1, block_x_2, block_y_2))
			return 1;
		else
			return 2;
	}
}

bool CheckCollision(int* delta_x, int* delta_y, int point_th, bool *reflect){
	int hit_object = -3; //wall:-2, bar:-1, block:i(0-block_num)
	int reflect_type = -1; //reflect x: 0, reflect y: 1
	int min_ratio = 2;
	int padding = points[point_th].get_radius()-1;
	if(*delta_y < 0){
		//hit top wall
		if(points[point_th].get_y()+(*delta_y) < padding){
			min_ratio = (points[point_th].get_y()-padding)/(*delta_y);
			hit_object = -2;
			reflect_type = 1;
		}
	} else if(*delta_y > 0 && points[point_th].get_y() < bar_top){
		//hit top of bar
		if(double buf = CheckCross(point_th, *delta_x, *delta_y, bar_left-padding, bar_top-padding, bar_right+padding, bar_top-padding) < min_ratio){
			min_ratio = buf;
			hit_object = -1;
			reflect_type = 1;
		}
	}

	if(*delta_x < 0){
		//hit left wall
		if(points[point_th].get_x()+(*delta_x) < padding){
			min_ratio = (points[point_th].get_x()-padding)/(*delta_x);
			hit_object = -2;
			reflect_type = 0;
		}
		//hit right of bar
		if(double buf = CheckCross(point_th, *delta_x, *delta_y, bar_right+padding, bar_top-padding, bar_right+padding, bar_bottom+padding) < min_ratio){
			min_ratio = buf;
			hit_object = -1;
			reflect_type = 0;
		}
	} else{
		//hit right wall
		if(points[point_th].get_x()+(*delta_x) > CLIENT_WIDTH-padding){
			min_ratio = (CLIENT_WIDTH-padding-points[point_th].get_x())/(*delta_x);
			hit_object = -2;
			reflect_type = 0;
		}
		//hit left of bar 
		if(double buf = CheckCross(point_th, *delta_x, *delta_y, bar_left-padding, bar_top-padding, bar_left-padding, bar_bottom+padding) < min_ratio){
			min_ratio = buf;
			hit_object = -1;
			reflect_type = 0;
		}
	}

	//hit block
	for(int i=0; i<MAX_BLOCK; i++){
		if(blocks[i].get_life() <= 0) continue;
		if(*delta_y < 0 && points[point_th].get_y() > blocks[i].get_bottom()){
			//hit bottom of block
			if(double buf = CheckCross(point_th, *delta_x, *delta_y, blocks[i].get_left()-padding, blocks[i].get_bottom()+padding, blocks[i].get_right()+padding, blocks[i].get_bottom()+padding) < min_ratio){
				min_ratio = buf;
				hit_object = i;
				reflect_type = 1;
			}
		} else if(*delta_y > 0 && points[point_th].get_y() < blocks[i].get_top()){
			//hit top of block
			if(double buf = CheckCross(point_th, *delta_x, *delta_y, blocks[i].get_left()-padding, blocks[i].get_top()-padding, blocks[i].get_right()+padding, blocks[i].get_top()-padding) < min_ratio){
				min_ratio = buf;
				hit_object = i;
				reflect_type = 1;
			}
		}
		if(*delta_x < 0 && points[point_th].get_x() > blocks[i].get_right()){
			//hit right of block
			if(double buf = CheckCross(point_th, *delta_x, *delta_y, blocks[i].get_right()+padding, blocks[i].get_top()-padding, blocks[i].get_right()+padding, blocks[i].get_bottom()+padding) < min_ratio){
				min_ratio = buf;
				hit_object = i;
				reflect_type = 0;
			}
		} else if(*delta_x > 0 && points[point_th].get_x() < blocks[i].get_left()){
			//hit left of block
			if(double buf = CheckCross(point_th, *delta_x, *delta_y, blocks[i].get_left()-padding, blocks[i].get_top()-padding, blocks[i].get_left()-padding, blocks[i].get_bottom()+padding) < min_ratio){
				min_ratio = buf;
				hit_object = i;
				reflect_type = 0;
			}
		}
	}


	if(min_ratio == 2){
		return false;
	} else{
		points[point_th].move((*delta_x)*min_ratio, (*delta_y)*min_ratio);
		*delta_x *= (1-min_ratio);
		*delta_y *= (1-min_ratio);
		if(reflect_type == 0){
			*delta_x = -(*delta_x);
			points[point_th].reflect_x();
		} else{
			*delta_y = -(*delta_y);
			points[point_th].reflect_y();
		}
		if(hit_object > -1) blocks[hit_object].hit();
		*reflect = true;
		if(min_ratio == 1) return false;
		return true;
	}
}

bool SetWindowSize(HWND hWnd){
	RECT rw, rc;
	::GetWindowRect(hWnd, &rw);
	::GetClientRect(hWnd, &rc);

	int new_height = (rw.bottom-rw.top)-(rc.bottom-rc.top)+CLIENT_HEIGHT;
	int new_width = (rw.right-rw.left)-(rc.right-rc.left)+CLIENT_WIDTH;

	hWndSize = rc.bottom-rc.top;
	wWndSize = rc.right-rc.left;

	return SetWindowPos(hWnd, NULL, 0, 0, new_width, new_height, SWP_NOMOVE | SWP_NOZORDER);
}

void prepareDoubleBuffering(HWND hWnd){
	HDC hdc;
	hdc = GetDC(hWnd);
	hBitmap = CreateCompatibleBitmap(hdc, CLIENT_WIDTH, CLIENT_HEIGHT);
	mhdc = CreateCompatibleDC(NULL);
	SelectObject(mhdc, hBitmap);
}

void SetFont(HDC hdc){
	font_title = CreateFont(hWndSize/16, hWndSize/16, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_SWISS, NULL);
	font_subtitle = CreateFont(hWndSize/24, hWndSize/24, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_SWISS, NULL);
	font_main = CreateFont(hWndSize/32, hWndSize/32, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_SWISS, NULL);
	SelectObject(hdc, font_title);
	GetTextExtentPoint32(hdc, "A", lstrlen("A"), &size_font_title);
	SelectObject(hdc, font_subtitle);
	GetTextExtentPoint32(hdc, "A", lstrlen("A"), &size_font_subtitle);
	SelectObject(hdc, font_main);
	GetTextExtentPoint32(hdc, "a", lstrlen("a"), &size_font_main);
}

void LoadBGM(){
	open_bgm.lpstrDeviceType = TEXT("MPEGVideo");
	open_bgm.lpstrElementName = TEXT("resource/bgm/start_bgm.mp3");
	if(mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, (DWORD_PTR)&open_bgm)){
		MessageBox(NULL, TEXT("open mp3 file failed!"), TEXT("Windows Desktop Guided Tour"), NULL);
		PostQuitMessage(0);
	}
	play_bgm.dwCallback = (DWORD)hWnd;
	status_bgm.dwItem = MCI_STATUS_MODE;
	mciSendCommand(open_bgm.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&status_bgm);
	MCI_DGV_SETAUDIO_PARMS set_bgm_volume;
	set_bgm_volume.dwItem = MCI_DGV_SETAUDIO_VOLUME;
	set_bgm_volume.dwValue = bgm_volume*5;
	if(mciSendCommand(open_bgm.wDeviceID, MCI_SETAUDIO, MCI_DGV_SETAUDIO_VALUE | MCI_DGV_SETAUDIO_ITEM, (DWORD_PTR)&set_bgm_volume)){
		MessageBox(NULL, TEXT("change volume of bgm failed!"), TEXT("Windows Desktop Guided Tour"), NULL);
		PostQuitMessage(0);
	}
}

void RelaodSEVolume(){
	se_select.change_volume();
	se_reflect.change_volume();
	se_break.change_volume();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){

	PAINTSTRUCT ps;
	HDC hdc;

	switch(message){
	case WM_CREATE:
		if(!SetWindowSize(hWnd)){
			MessageBox(NULL, "Change size of window is failed!", "Windows Desktop Guided Tour", NULL);
			PostQuitMessage(0);
		}
		flag = STATE_STARTMENU;
		now_selected = 0;
		prepareDoubleBuffering(hWnd);
		hdc = GetDC(hWnd);
		SetFont(hdc);
		
		LoadBGM();
		mciSendCommand(open_bgm.wDeviceID, MCI_PLAY, MCI_NOTIFY, (DWORD_PTR)&play_bgm);
		se_select.load();

		return 0;
		break;
	case MM_MCINOTIFY:
		if(lParam == open_bgm.wDeviceID){
			if(wParam == MCI_NOTIFY_SUCCESSFUL){
				mciSendCommand(open_bgm.wDeviceID, MCI_SEEK, MCI_SEEK_TO_START, 0);
				mciSendCommand(open_bgm.wDeviceID, MCI_PLAY, MCI_NOTIFY, (DWORD_PTR)&status_bgm);
			}
		}
	case WM_TIMER:
		if(point_num <= 0){
			//game over
			KillTimer(hWnd, 1);
			DeleteObj();
			flag = STATE_GAMEOVER;
			if(!points.empty()){
				MessageBox(hWnd, "erase point vector failed!", "Windows Desktop Guided Tour", NULL);
				PostQuitMessage(0);
			}
			if(!blocks.empty()){
				MessageBox(hWnd, "erase block vector failed!", "Windows Desktop Guided Tour", NULL);
				PostQuitMessage(0);
			}
			InvalidateRect(hWnd, NULL, TRUE);
			EndGame();
		} else if(block_num <= 0){
			//game clear
			KillTimer(hWnd, 1);
			DeleteObj();
			flag = STATE_GAMECLEAR;
			if(!points.empty()){
				MessageBox(hWnd, "erase point vector failed!", "Windows Desktop Guided Tour", NULL);
				PostQuitMessage(0);
			}
			if(!blocks.empty()){
				MessageBox(hWnd, "erase block vector failed!", "Windows Desktop Guided Tour", NULL);
				PostQuitMessage(0);
			}
			InvalidateRect(hWnd, NULL, TRUE);
			EndGame();
		} else{
			if(bar_left+bar_vec*BAR_SPD < 0){
				bar_right -= bar_left;
				bar_left = 0;
			} else if(bar_right+bar_vec*BAR_SPD > wWndSize){
				bar_left += wWndSize-bar_right;
				bar_right = wWndSize;
			} else{
				bar_left += bar_vec*BAR_SPD;
				bar_right += bar_vec*BAR_SPD;
			}
			int n = point_max;
			bool isReflect = false;
			for(int i=0; i<n; i++){
				if(!points[i].isSave()) continue;
				if(points[i].check_drop()) continue;
				//check collision
				int delta_x = points[i].get_spd_x();
				int delta_y = points[i].get_spd_y();
				while(CheckCollision(&delta_x, &delta_y, i, &isReflect)){

				}
				points[i].move(delta_x, delta_y);
			}
			if(isReflect){
				se_reflect.play();
			}
			InvalidateRect(hWnd, NULL, FALSE);
		}
		UpdateWindow(hWnd);
		break;
	case WM_KEYDOWN:
		switch(wParam){
		case VK_RIGHT: 
			if(flag == STATE_INGAME){
				bar_vec = 1;
			} else if(flag==STATE_OPTION && now_selected==0){
				if(bgm_volume < 200) bgm_volume++;
				InvalidateRect(hWnd, NULL, FALSE);
				MCI_DGV_SETAUDIO_PARMS set_bgm_volume;
				set_bgm_volume.dwItem = MCI_DGV_SETAUDIO_VOLUME;
				set_bgm_volume.dwValue = bgm_volume*5;
				mciSendCommand(open_bgm.wDeviceID, MCI_SETAUDIO, MCI_DGV_SETAUDIO_VALUE | MCI_DGV_SETAUDIO_ITEM, (DWORD_PTR)&set_bgm_volume);
				se_select.play();
			} else if(flag==STATE_OPTION && now_selected==1){
				if(se_volume < 200) se_volume++;
				RelaodSEVolume();
				InvalidateRect(hWnd, NULL, FALSE);
				se_select.play();
			}
			break;
		case VK_LEFT:
			if(flag == STATE_INGAME){
				bar_vec = -1;
			} else if(flag==STATE_OPTION && now_selected==0){
				if(bgm_volume > 0) bgm_volume--;
				InvalidateRect(hWnd, NULL, FALSE);
				MCI_DGV_SETAUDIO_PARMS set_bgm_volume;
				set_bgm_volume.dwItem = MCI_DGV_SETAUDIO_VOLUME;
				set_bgm_volume.dwValue = bgm_volume*5;
				mciSendCommand(open_bgm.wDeviceID, MCI_SETAUDIO, MCI_DGV_SETAUDIO_VALUE | MCI_DGV_SETAUDIO_ITEM, (DWORD_PTR)&set_bgm_volume);
				se_select.play();
			} else if(flag==STATE_OPTION && now_selected==1){
				if(se_volume > 0) se_volume--;
				RelaodSEVolume();
				InvalidateRect(hWnd, NULL, FALSE);
				se_select.play();
			}
			break;
		case VK_UP:
			if(flag != STATE_INGAME){
				if(now_selected > 0) now_selected--;
				InvalidateRect(hWnd, NULL, TRUE);
				se_select.play();
			}
			break;
		case VK_DOWN:
			if(flag == STATE_STARTMENU){
				if(now_selected < STARTMENU_NUM-1){
					now_selected++;
					InvalidateRect(hWnd, NULL, TRUE);
					se_select.play();
				}
			} else if(flag == STATE_GAMECLEAR){
				if(now_selected < GAMECLEARMENU_NUM-1){
					now_selected++;
					InvalidateRect(hWnd, NULL, TRUE);
					se_select.play();
				}
			} else if(flag == STATE_GAMEOVER){
				if(now_selected < GAMEOVERMENU_NUM-1){
					now_selected++;
					InvalidateRect(hWnd, NULL, TRUE);
					se_select.play();
				}
			} else if(flag == STATE_OPTION){
				if(now_selected < OPTIONMENU_NUM-1){
					now_selected++;
					InvalidateRect(hWnd, NULL, TRUE);
					se_select.play();
				}
			}
			break;
		case VK_RETURN: 
			if(flag==STATE_STARTMENU){
				if(now_selected==0){
					StartGame();
					SetTimer(hWnd, 1, 10, NULL);
				} else if(now_selected==1){
					now_selected = 0;
					flag = STATE_OPTION;
					InvalidateRect(hWnd, NULL, TRUE);
				} else if(now_selected==2){
					DeleteObject(hbbg);
					PostQuitMessage(0);
				}
			} else if(flag==STATE_GAMECLEAR || flag==STATE_GAMEOVER){
				if(now_selected==0){
					StartGame();
					SetTimer(hWnd, 1, 10, NULL);
				} else if(now_selected==1){
					now_selected = 0;
					flag = STATE_STARTMENU;
					LoadBGM();
					mciSendCommand(open_bgm.wDeviceID, MCI_PLAY, MCI_NOTIFY, (DWORD_PTR)&play_bgm);
					InvalidateRect(hWnd, NULL, TRUE);
				} else if(now_selected==2){
					DeleteObject(hbbg);
					PostQuitMessage(0);
				}
			} else if(flag == STATE_OPTION){
				if(now_selected==2){
					now_selected = 0;
					flag = STATE_STARTMENU;
					InvalidateRect(hWnd, NULL, TRUE);
				}
			}
			break;
		case VK_ESCAPE:
			if(flag==STATE_GAMECLEAR || flag==STATE_GAMEOVER || flag==STATE_STARTMENU){
				DeleteObject(hbbg);
				PostQuitMessage(0);
			} else if(flag == STATE_OPTION){
				now_selected = 0;
				flag = STATE_STARTMENU;
				InvalidateRect(hWnd, NULL, TRUE);
			}
			break;
		}
		return 0;
		break;
	case WM_KEYUP:
		if(flag == STATE_INGAME){
			bar_vec = 0;
		}
		return 0;
		break;
	case WM_PAINT:
		if(flag == STATE_INGAME){
			hb = (HBRUSH)SelectObject(mhdc, hbbg);
			PatBlt(mhdc, 0, 0, CLIENT_WIDTH, CLIENT_HEIGHT, PATCOPY);
			SelectObject(mhdc, hwhiteb);
			SelectObject(mhdc, hwhitep);
			Rectangle(mhdc, bar_left, bar_top, bar_right, bar_bottom);
			for(int i=0; i<point_max; i++){
				printf("%d, %d\n", points[i].get_x(), points[i].get_y());
				if(!points[i].isSave()) continue;
				int radius = points[i].get_radius();
				Ellipse(mhdc, points[i].get_x()-radius, points[i].get_y()-radius, points[i].get_x()+radius, points[i].get_y()+radius);
			}
			for(int i=0; i<MAX_BLOCK; i++){
				int life = blocks[i].get_life();
				if(life <= 0) continue;
				hb = CreateSolidBrush(blocks[i].get_color(life));
				SelectObject(mhdc, hb);
				SelectObject(mhdc, hp);
				Rectangle(mhdc, blocks[i].get_left(), blocks[i].get_top(), blocks[i].get_right(), blocks[i].get_bottom());
				DeleteObject(hb);
			}
			hdc = BeginPaint(hWnd, &ps);
			BitBlt(hdc, 0, 0, CLIENT_WIDTH, CLIENT_HEIGHT, mhdc, 0, 0, SRCCOPY);
			EndPaint(hWnd, &ps);
		} else if(flag == STATE_STARTMENU){
			RECT rect;
			hdc = BeginPaint(hWnd, &ps);
			GetClientRect(hWnd, &rect);
			SetTextColor(hdc, RGB(255, 255, 255));
			SetBkColor(hdc, RGB(0, 0, 0));
			SetTextAlign(hdc, TA_CENTER | TA_TOP);
			SelectObject(hdc, font_title);
			TextOut(hdc, wWndSize/2-size_font_title.cx*1.75, hWndSize/3, "BREAK", lstrlen("BREAK"));
			TextOut(hdc, wWndSize/2+size_font_title.cx*2.75, hWndSize/3, "OUT", lstrlen("OUT"));
			SelectObject(hdc, font_main);
			TextOut(hdc, wWndSize/2, hWndSize*2/3, "game start", lstrlen("game start"));
			TextOut(hdc, wWndSize/2, hWndSize*2/3+size_font_main.cy*1.5, "option", lstrlen("option"));
			TextOut(hdc, wWndSize/2, hWndSize*2/3+size_font_main.cy*3, "quit game", lstrlen("quit game"));
			TextOut(hdc, wWndSize/2-size_font_main.cx*7, hWndSize*2/3+size_font_main.cy*1.5*now_selected, "->", lstrlen("->"));
			EndPaint(hWnd, &ps);
		} else if(flag == STATE_GAMECLEAR){
			RECT rect;
			hdc = BeginPaint(hWnd, &ps);
			GetClientRect(hWnd, &rect);
			SetTextColor(hdc, RGB(255, 255, 255));
			SetBkColor(hdc, RGB(0, 0, 0));
			SetTextAlign(hdc, TA_CENTER | TA_TOP);
			SelectObject(hdc, font_title);
			TextOut(hdc, wWndSize/2-size_font_title.cx*2.75, hWndSize/3, "GAME", lstrlen("GAME"));
			TextOut(hdc, wWndSize/2+size_font_title.cx*2.25, hWndSize/3, "CLEAR", lstrlen("CLEAR"));
			SelectObject(hdc, font_main);
			TextOut(hdc, wWndSize/2, hWndSize*2/3, "play again", lstrlen("play again"));
			TextOut(hdc, wWndSize/2, hWndSize*2/3+size_font_main.cy*1.5, "return start", lstrlen("return start"));
			TextOut(hdc, wWndSize/2, hWndSize*2/3+size_font_main.cy*3, "quit game", lstrlen("quit game"));
			TextOut(hdc, wWndSize/2-size_font_main.cx*8, hWndSize*2/3+size_font_main.cy*1.5*now_selected, "->", lstrlen("->"));
			EndPaint(hWnd, &ps);
		} else if(flag == STATE_GAMEOVER){
			RECT rect;
			hdc = BeginPaint(hWnd, &ps);
			SetTextColor(hdc, RGB(255, 255, 255));
			SetBkColor(hdc, RGB(0, 0, 0));
			GetClientRect(hWnd, &rect);
			SetTextAlign(hdc, TA_CENTER | TA_TOP);
			SelectObject(hdc, font_title);
			TextOut(hdc, wWndSize/2-size_font_title.cx*2.25, hWndSize/3, "GAME", lstrlen("GAME"));
			TextOut(hdc, wWndSize/2+size_font_title.cx*2.25, hWndSize/3, "OVER", lstrlen("OVER"));
			SelectObject(hdc, font_main);
			TextOut(hdc, wWndSize/2, hWndSize*2/3, "play again", lstrlen("play again"));
			TextOut(hdc, wWndSize/2, hWndSize*2/3+size_font_main.cy*1.5, "return start", lstrlen("return start"));
			TextOut(hdc, wWndSize/2, hWndSize*2/3+size_font_main.cy*3, "quit game", lstrlen("quit game"));
			TextOut(hdc, wWndSize/2-size_font_main.cx*8, hWndSize*2/3+size_font_main.cy*1.5*now_selected, "->", lstrlen("->"));
			EndPaint(hWnd, &ps);
		} else if(flag == STATE_OPTION){
			RECT rect;
			hdc = BeginPaint(hWnd, &ps);
			GetClientRect(hWnd, &rect);
			SetTextColor(hdc, RGB(255, 255, 255));
			SetBkColor(hdc, RGB(0, 0, 0));
			TCHAR text_bgm[128], text_se[128];
			wsprintf(text_bgm, TEXT("bgm < %03d >"), bgm_volume);
			wsprintf(text_se, TEXT("se  < %03d >"), se_volume);
			SetTextAlign(hdc, TA_CENTER | TA_TOP);
			SelectObject(hdc, font_title);
			TextOut(hdc, wWndSize/2, hWndSize/6, "OPTION", lstrlen("OPTION"));
			SelectObject(hdc, font_subtitle);
			int height = hWndSize/6+size_font_main.cy*4;
			TextOut(hdc, wWndSize/3, height, "SOUND", lstrlen("SOUND"));
			height += size_font_main.cy*2;
			SelectObject(hdc, font_main);
			TextOut(hdc, wWndSize/2, height, text_bgm, lstrlen(text_bgm));
			TextOut(hdc, wWndSize/2, height+size_font_main.cy*1.5, text_se, lstrlen(text_se));
			TextOut(hdc, wWndSize/2, height+size_font_main.cy*3, "return start", lstrlen("return start"));
			TextOut(hdc, wWndSize/2-size_font_main.cx*8, height+size_font_main.cy*1.5*now_selected, "->", lstrlen("->"));
			EndPaint(hWnd, &ps);
		}
		return 0;
		break;
	case WM_SIZE:
		RECT rc;
		::GetClientRect(hWnd, &rc);
		hWndSize = rc.bottom-rc.top;
		wWndSize = ASPECT_RATIO * hWndSize;
		hdc = GetDC(hWnd);
		SetFont(hdc);
		InvalidateRect(hWnd, NULL, TRUE);
		break;
	case WM_DESTROY:
		DeleteObj();
		DeleteObject(hbbg);
		mciSendCommand(open_bgm.wDeviceID, MCI_CLOSE, 0, 0);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
	hbbg = CreateSolidBrush(RGB(0, 0, 0));
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = WndProc;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 0;
	wcex.hInstance      = hInstance;
	wcex.hIcon          = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName   = NULL;
	wcex.lpszClassName  = szWindowClass;
	wcex.hIconSm        = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	wcex.hbrBackground  = hbbg;

	if(!RegisterClassEx(&wcex)){
		MessageBox(NULL, "Call to RegisterClassEx failed!", "Windows Desktop Guide Tour", NULL);
		return 1;
	}

	//store instance handle in our global variable
	hInst = hInstance;

	// the parameters to CreateWindow explained:
	// szWindowClass : the name of application
	// szTitle : the text that appears in the title bar
	// WS_OVERLAPPEDWINDOW : the type of window to create
	// CW_USEDEFAULT, CW_USEDEFAULT : StartGameial position (x, y)
	// wWndSize, hWndSize : StartGameial size (width, height)
	// NULL : the parent of this window
	// NULL : the application dows not have a menu bar
	// hInstance : the first parameter from WinMain
	// NULL : not used in this application

	HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, wWndSize, hWndSize, NULL, NULL, hInstance, NULL);

	if(!hWnd){
		MessageBox(NULL, "Call to CreateWindow failed!", "Windows Desktop Guided Tour", NULL);
		return 1;
	}

	// the parameters to ShowWindow explained:
	// hWnd: the value returned from CreateWindow
	// nCmdShow : the fourth parameter from WinMain
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// main message loop
	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}