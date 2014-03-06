#include <stdio.h>
#include <xenos/xenos.h>
#include <console/console.h>
#include <xenon_sound/sound.h>
extern "C"{
        #include "SDL_gfxPrimitives.h"
}
#include "vecx.h"
#include "osint.h"
#include <SDL/SDL_image.h>
#include <input/input.h>
#include <libfat/fat.h>
#include <zlx/Browser.h>
#include <zlx/Draw.h>
#include <zlx/Hw.h>
#include <dirent.h>
#define STICK_THRESHOLD 12000
#define EMU_TIMER 30 /* the emulators heart beats at 20 milliseconds */
//XenosSurface muestra elementos en pantalla
XenosSurface * pantallaXenon;

ZLX::Browser Menu;

static SDL_Surface *screen = NULL;
static SDL_Surface *overlay= NULL;

static long screenx;
static long screeny;
static long scl_factor;
static long offx;
static long offy;



void osint_render(void){
	SDL_FillRect(screen, NULL, 0);

	int v;
	for(v = 0; v < vector_draw_cnt; v++){
		Uint8 c = vectors_draw[v].color * 256 / VECTREX_COLORS;
		aalineRGBA(screen,
				offx + vectors_draw[v].x0 / scl_factor,
        offy + vectors_draw[v].y0 / scl_factor,
				offx + vectors_draw[v].x1 / scl_factor,
        offy + vectors_draw[v].y1 / scl_factor,
				c, c, c, 0xff);
	}
        // Draw overlay image
    if (overlay != NULL)
    {
        SDL_Rect src, dst;
        dst.x = src.x = 0;  dst.y = src.y = 0;
        dst.w = src.w = (Uint16)screenx;  dst.h = src.h = (Uint16)screeny;
        SDL_BlitSurface(overlay, &src, screen, &dst);
    }
	SDL_Flip(screen);
}
/*esta funcion comprueba si el directorio correspondiente de roms esta vacio*/
BOOL directorioVacio(){
    BOOL vacio;
    DIR *d=opendir("uda:/Vecx-360_roms/");
    int contador=0;
    struct dirent *ds;
    while((ds=readdir(d))!=NULL){
        contador++;
    }
    closedir(d);
    if (contador>2){
        vacio=false;
    }
    else{
        vacio=true;
    }
    return vacio;
}
/*comprueba que el directorio correspondiente exista y si no esta vacio*/
BOOL comprobarRoms(){
    BOOL existe;
    DIR * roms =opendir("uda:/Vecx-360_roms/");
    if(roms==NULL||directorioVacio()==true){
        closedir(roms);
        existe=false;
    }
    else{
        closedir(roms);
        existe=true;
    }
    return existe;
}

static char *romfilename = "rom.dat";
static char *cartfilename = NULL;

/*esta funcion comprueba que el archivo seleccionado sea una rom de vectrex*/

BOOL comprobarRom(){
    BOOL correcto;
    char *lectura="";
    char *leido="g GCE";
    char *leido2="MINE";
    FILE *rom=fopen(cartfilename,"rb");
    lectura=fgets(lectura,6,rom);
    fclose(rom);
    if (lectura!=NULL){
            if (strcmp(lectura,leido)==0){
                correcto=true;
                return correcto;
            }
            else{
                FILE *rom=fopen(cartfilename,"rb");
                fseek(rom,6*sizeof(char),SEEK_SET);
                lectura=fgets(lectura,5,rom);
                fclose(rom);
                if(strcmp(lectura,leido2)==0){
                    correcto=true;
                    return correcto;
                }
                else{
                correcto=false;
                return correcto;
                }
            }
        }
    else
        correcto = false;
    return correcto;
}

void iniciarRom(){
    FILE *f=fopen(romfilename,"rb");
    if(fread(rom, 1, sizeof (rom), f) != sizeof(rom)){
	printf("Invalid rom length\n");
	exit(EXIT_FAILURE);
    }
    fclose(f);
    memset(cart, 0, sizeof (cart));
    if(cartfilename){
        FILE *f=fopen(cartfilename,"rb");
	if(!(f = fopen(cartfilename, "rb"))){
			perror(cartfilename);
			exit(EXIT_FAILURE);
		}
		fread(cart, 1, sizeof (cart), f);
		fclose(f);
	}
    char buffer[256];
        char* period;
        // Prepare name for overlay image: same as cartridge name but with ".png" extension
        strcpy(buffer, cartfilename);
        period = strrchr(buffer, '.');
        strcpy(period, ".png");

        // Seek for overlay image, load if found
        overlay = NULL;
		if(f = fopen(buffer, "rb")){
    		fclose(f);

            overlay = IMG_Load(buffer);
        }
}

static void init(){
    FILE *f;
    if(!(f= fopen(romfilename, "rb"))){
        perror(romfilename);
        exit(EXIT_FAILURE);
    }
    if(fread(rom, 1, sizeof (rom), f) != sizeof(rom)){
	printf("Invalid rom length\n");
	exit(EXIT_FAILURE);
    }
    fclose(f);
    char buffer[256];
        char* period;
        // Prepare name for overlay image: same as cartridge name but with ".png" extension
        strcpy(buffer, romfilename);
        period = strrchr(buffer, '.');
        strcpy(period, ".png");

        // Seek for overlay image, load if found
        overlay = NULL;
		if(f = fopen(buffer, "rb")){
    		fclose(f);

            overlay = IMG_Load(buffer);
        }
}

void resize(int width, int height){
	long sclx, scly;

	screenx = width;
	screeny = height;
	screen = SDL_SetVideoMode(screenx, screeny, 0, SDL_SWSURFACE | SDL_RESIZABLE);

	sclx = ALG_MAX_X / screen->w;
	scly = ALG_MAX_Y / screen->h;

	scl_factor = sclx > scly ? sclx : scly;

	offx = (screenx - ALG_MAX_X / scl_factor) / 2;
	offy = (screeny - ALG_MAX_Y / scl_factor) / 2;
}


static void readevents(){
//    // En esta funcion leemos los movimientos del mando de la xbox 360
//    // Las funciones de teclado se han eliminado y se han asignado
//    // a los botones Y,X,A y B del mando así como a la cruceta
//    
    static struct controller_data_s oldc;
  
    int pulsacion = 0;
    while(pulsacion == 0)
    {    
        static struct controller_data_s c;
      if(get_controller_data(&c,0))
      { 
        
        //Tecla A del teclado correponde con el boton del mando Y 
        if ((c.y) && (!oldc.y))
        {
            snd_regs[14] &= ~0x01; //pulsado
        }
        if ((!c.y) && (oldc.y))
        {
          snd_regs[14] |= 0x01; //soltado  
        }
        
        //Tecla S del teclado correponde con el boton del mando X 
        if ((c.x) && (!oldc.x))
        {
            snd_regs[14] &= ~0x02; //pulsado
        }
        if ((!c.x) && (oldc.x))
        {
          snd_regs[14] |= 0x02; // soltado
        }
        
        //Tecla D del teclado correponde con el boton del mando A 
        if ((c.a) && (!oldc.a))
        {
            snd_regs[14] &= ~0x04;
        }
        if ((!c.a) && (oldc.a))
        {
          snd_regs[14] |= 0x04;  
        }    
        
        //Tecla F del teclado correponde con el boton del mando B
        if ((c.b) && (!oldc.b))
        {
            snd_regs[14] &= ~0x08; //pulsado 
        }
        if ((!c.b) && (oldc.b))
        {
          snd_regs[14] |= 0x08;  // Soltado
        }
        
        // Movimiento de la cruceta del mando de la xbox 360
        
        //Arriba 
        if ((c.up) && (!oldc.up))
        {
            alg_jch1 = 0xff;
        }
        if ((!c.up) && (oldc.up))
        {
          alg_jch1 = 0x80;  
        }    
        
        //Abajo
        if ((c.down) && (!oldc.down))
        {
            alg_jch1 = 0x00;
        }
        if ((!c.down) && (oldc.down))
        {
          alg_jch1 = 0x80;  
        }    
        
        //Izquierda
        if ((c.left) && (!oldc.left))
        {
            alg_jch0 = 0x00;
        }
        if ((!c.left) && (oldc.left))
        {
          alg_jch0 = 0x80;  
        }    
        
        //Derecha
        if ((c.right) && (!oldc.right))
        {
            alg_jch0 = 0xff;
        }
        if ((!c.right) && (oldc.right))
        {
          alg_jch0 = 0x80;
        } 
        if(c.back)
            exit(0);
        //Analogicos
        if(!c.up&&!c.down&&!c.left&&!c.right){
        //Izquierda
        if(c.s1_x<-STICK_THRESHOLD){
            alg_jch0 = 0x00;
        }
        //derecha
        if(c.s1_x>STICK_THRESHOLD){
            alg_jch0 = 0xff;
        }
        //Arriba
        if(c.s1_y>STICK_THRESHOLD){
            alg_jch1 = 0xff;
        }
        //Abajo
        if(c.s1_y<-STICK_THRESHOLD){
            alg_jch1 = 0x00;
        }
        //centro
        if((c.s1_x<STICK_THRESHOLD && c.s1_x>-STICK_THRESHOLD)
                &&(c.s1_y<STICK_THRESHOLD && c.s1_y>-STICK_THRESHOLD)){
            alg_jch0 = 0x80;
            alg_jch1=0x80;
        }
        }
        oldc=c;
      }
      pulsacion=1;
      usb_do_poll();
    }
}

void osint_emuloop(){
	Uint32 next_time = SDL_GetTicks() + EMU_TIMER;
	vecx_reset();
	for(;;){
		vecx_emu((VECTREX_MHZ / 1000) * EMU_TIMER, 0);
                readevents();

		{
			Uint32 now = SDL_GetTicks();
			if(now < next_time)
				SDL_Delay(next_time - now);
			else
				next_time = now;
			next_time += EMU_TIMER;
		}
	}
}
void ActionLaunchFile(char * filename){
    cartfilename=filename;
    if (comprobarRom()){
        SDL_Init(SDL_INIT_VIDEO&&SDL_INIT_JOYSTICK);
        resize(240, 320);
        SDL_ShowCursor(0);
        iniciarRom();
        osint_emuloop();
        SDL_Quit();
    }
    else{
        char bufferTexto[1024];
        sprintf(bufferTexto,"Could not load file:\n\n%s\n\nIt is probably not a Vectrex rom.",filename);
        Menu.Alert(bufferTexto);
    }
}
void iniciarMenu(){
    ZLX::Hw::SystemInit(ZLX::INIT_USB);
    ZLX::Hw::SystemPoll();
    Menu.SetLaunchAction(ActionLaunchFile);
    Menu.Run("/");
}
void iniciarRomDefecto(){
    SDL_Init(SDL_INIT_VIDEO&&SDL_INIT_JOYSTICK);
    resize(240,230);
    SDL_ShowCursor(0);
    init();
    osint_emuloop();
    SDL_Quit();
}

int main(){
    xenos_init(VIDEO_MODE_AUTO);
    console_init();
    xenon_make_it_faster(XENON_SPEED_FULL);
    xenon_sound_init();
    usb_init();
    usb_do_poll();
    if(!fatInitDefault()){
        perror("no se ha montado fat");
        exit(EXIT_FAILURE);
    }
    if(comprobarRoms()==false){
        iniciarRomDefecto();
    }
    else{
        iniciarMenu();
    }
    return 0;
}
