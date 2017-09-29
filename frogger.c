#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_mixer.h>
#include <assert.h>
#include <emscripten.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
/*
A ENVOYER :

PREMIERE DU PROJET A ENVOYER VENDREDI 27 SOIR.
FINAL VERSION LE 9 DECEMBRE. (TECHNICAL REPORT IN ENGLISH //RULES, HOW WORKS, GOALS, EXPLAIN ALGORITHM) + (PRESENTATION IN ENGLISH)
(POST IT ON THE MOODLE => CODE SOURCE / MEDIA FILES / WORKING VERSION / ARCHIVE IT WITH TAR+GZIP OR ZIP)
*/
enum {WIDTH = 15};
enum {HEIGHT = 16};
enum {ELMTSIZE = 34};
enum {LEVEL, GAMEOVER, WIN, NSCREEN};
enum {EMPTY, STREET, WATER, PLATEFORM, HERBE, NOPE, YEP, NENT};
enum {NORTH, SOUTH, EAST, WEST, NDIR};
enum {TEST, TETEBUCHE, MIDBUCHE, QUEUEBUCHE, TORTUE, QUEUECROCO, MIDCROCO, TETECROCO, VOITURE, VOITURE2, QUEUECAMION, TETECAMION, AVION, NBENTITE};

typedef struct 
{
  int x;
  int y;
  int dir;
  int ennemy;
  int vitesse;
  int count_down;
  int heroe;
  int val;
} Entity;

typedef struct 
{
  int x;
  int y;
} Point;

Point coor[NDIR] = { {0, -1}, {0, 1}, {1, 0}, {-1, 0} };

typedef struct //the structure which contains 
{
  // Menu ou niveau à instancier  
    int lvl;
    int board[HEIGHT][WIDTH];
    Entity board_ent[HEIGHT][WIDTH+1];
    //Timer de déplacement des ennemis
    Uint32 currentTime;
    Uint32 precTime;
    //Timer du son main
    int booleanson;
    int booleansonfin;
    Uint32 currentTime2;
    Uint32 precTime2;

    SDL_Surface* img_screen;
    SDL_Surface* img_crono;
    SDL_Surface* img_crononul;
    int crono;
    SDL_Surface* img_menu;
    int boulmenu;
    SDL_Surface* img_new_partie;

    SDL_Surface* img_nombre;
    SDL_Surface* img_lblscore;
    SDL_Rect clip[11];
    int score;
    int boulscore;

    SDL_Surface* img_empty;
    SDL_Surface* img_heroe[4];
    SDL_Surface* img_water;
    SDL_Surface* img_street;

    SDL_Surface* img_life;
    SDL_Surface* img_game_over;
    SDL_Surface* img_herbe;
    SDL_Surface* img_nope;
    SDL_Surface* img_yep;
    SDL_Surface* img_victoire;
    SDL_Surface* img_voiture;
    SDL_Surface* img_voiture2;
    SDL_Surface* img_voiture3;
    SDL_Surface* img_voiture_g;
    SDL_Surface* img_voiture2_g;
    SDL_Surface* img_voiture3_g;
    SDL_Surface* img_camion[2];
    SDL_Surface* img_camion_g[2];
    SDL_Surface* img_avion;
    SDL_Surface* img_avion_g;
    SDL_Surface* img_buche[3];
    SDL_Surface* img_tortue[13];
    SDL_Surface* img_tortue_g[13];
    SDL_Surface* img_croco[3];
    SDL_Surface* img_croco2;
    SDL_Surface* img_croco_g[3];
    SDL_Surface* img_croco2_g;
    
    Entity heroe;

    int life;
    int victoire;
    int vincentTesUneBranque;
    int niveau;

    Mix_Music *music;
    Mix_Music *music2;
    Mix_Music *music3;
    Mix_Music *music4;
    Mix_Music *music_victoire;
    int music_vicint;
    Mix_Chunk *save_sound;
    Mix_Chunk *frogdie;
    Mix_Chunk *stage_clear;
    Mix_Chunk *game_over;
    Mix_Chunk *bump;
    Mix_Chunk *one_up;
    Mix_Chunk *etoile;

    int cheatmode;
} Data;

// ************************************

SDL_Surface* load(const char* fileName) 
{
  SDL_Surface *image = IMG_Load(fileName);
  if (!image)
  {
     printf("IMG_Load: %s\n", IMG_GetError());
     return 0;
  }
  return image;
}

void lire (Data *data, int nb)
{
  FILE* fichier;
  //char * c;
  //itoa(nb, c);
  if (nb == 0)
    fichier = fopen("level/1", "r+");
  if (nb == 1)
    fichier = fopen("level/2", "r+");
  if (nb < 0 || nb > 1)
    return; 

  int i, j;

  while(!feof(fichier))
  {
    for (i = 0; i < HEIGHT; i++)
    {
      for(j = 0; j < WIDTH; j++)
      {
        fscanf(fichier, "%d, ", &data->board[i][j]);
      }
    }
  }
  fclose(fichier);
}

void apply_surface( int x, int y, SDL_Surface* source, SDL_Surface* destination, SDL_Rect* clip)
{
    SDL_Rect offset;
    offset.x = x;
    offset.y = y;
 
    //On blitte la surface
    SDL_BlitSurface( source, clip, destination, &offset );
}

void gain_point (Data* data, int a, int boul)//the function which will increase your score
{
  if(boul == 1)
  {
    data->score += a * (2 * data->niveau) + (3*data->crono);
  }
  else
    data->score += a * (2 * data->niveau);
  
  //Affiche le nouveau score
  int i, j, x;

  if(data->score == 0)
    apply_surface( WIDTH/2 * ELMTSIZE, 8, data->img_nombre, data->img_screen, &data->clip[0]);
  else
    for(i = data->score, j = 0; i > 0; i /=10, j-=15)
    {
      x = i % 10;
      apply_surface( WIDTH/2 * ELMTSIZE + j, 8, data->img_nombre, data->img_screen, &data->clip[10]);
      apply_surface( WIDTH/2 * ELMTSIZE + j, 8, data->img_nombre, data->img_screen, &data->clip[x]);
    }

  SDL_Rect position;
  position.x = 0;
  position.y = 0;
  SDL_BlitSurface (data->img_lblscore, NULL, data->img_screen, &position);
}

int line_for_ent[8] = {4,5,6,7,9,10,11,12};

void remplir_board_ent (Data* data)//the function which will randomly create a setup of ennemies and platforms
{
  int i, j, k, l, rdm_ent, rdm_bin, size, newVal;
  int tabrdm[7][15] = {
  {VOITURE, 0, 0, 0, 0, 0, VOITURE, 0, 0, 0, 0, VOITURE, 0, 0, 0},
  {VOITURE2, 0, 0, 0, 0, VOITURE2, 0, 0, 0, 0, VOITURE2, 0, 0, 0, 0},
  {QUEUECAMION, TETECAMION, 0, 0, 0, 0, 0, QUEUECAMION, TETECAMION, 0, 0, 0, 0, 0, 0},
  {AVION, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {TETEBUCHE, MIDBUCHE, QUEUEBUCHE, 0, 0, TETEBUCHE, MIDBUCHE, QUEUEBUCHE, 0, 0, TETEBUCHE, MIDBUCHE, QUEUEBUCHE, 0, 0},
  {QUEUECROCO, MIDCROCO, TETECROCO, 0, 0, QUEUECROCO, MIDCROCO, TETECROCO, 0, 0, QUEUECROCO, MIDCROCO, TETECROCO, 0, 0},
  {TORTUE, TORTUE, TORTUE, 0, 0, 0, 0, TORTUE, TORTUE, TORTUE, 0, 0, 0, 0, 0}};

  for(i=0;i<4;i++)
  {
    k=rand()%4;
    l=rand()%15;
    for(j=0;j<WIDTH;j++)
    {
      newVal=tabrdm[k][(j+l)%15];

      if(newVal==QUEUECAMION && (i&1))
        newVal=TETECAMION;
      else if(newVal==TETECAMION && (i&1))
        newVal=QUEUECAMION;

      Entity newEntity;
      if(newVal>=TETECROCO)
      {
        newEntity.ennemy=1;
      }
      else
      {
        newEntity.ennemy=0;   
      }
      newEntity.val=newVal;
      data->board_ent[9+i][j]=newEntity;
    }
  }
  printf("Niveau : %d\n",data->niveau);
  for(i=0;i<4;i++)
  { 
    if(data->niveau==1)
      k=4;
    else if(data->niveau==2)
      k=(rand()%2+4);
    else
      k=(rand()%3)+4;
    l=rand()%15;
    for(j=0;j<WIDTH;j++)
    {
      newVal=tabrdm[k][(j+l)%15];

      if(newVal==QUEUECROCO && (i&1))
        newVal=TETECROCO;
      else if(newVal==TETECROCO && (i&1))
        newVal=QUEUECROCO;

      Entity newEntity;
      if(newVal>=TETECROCO)
      {
        newEntity.ennemy=1;
      }
      else
      {
        newEntity.ennemy=0;   
      }
      newEntity.val=newVal;
      data->board_ent[4+i][j]=newEntity;
    }
  }
}

void board_entity (Data * data)
{
  int x, y;
  for(x = 0; x < WIDTH; x++) 
    {
      for(y = 0; y < HEIGHT; y++) 
      {
        SDL_Rect position;
        position.x = x * ELMTSIZE;
        position.y = y * ELMTSIZE;
        if(data->board_ent[y][x].val == VOITURE)
          SDL_BlitSurface (data->img_voiture, NULL, data->img_screen, &position);
        else if(data->board_ent[y][x].val == VOITURE2)
          SDL_BlitSurface (data->img_voiture2, NULL, data->img_screen, &position);
        else if(data->board_ent[y][x].val == TETECAMION)
            SDL_BlitSurface (data->img_camion[0], NULL, data->img_screen, &position);
        else if(data->board_ent[y][x].val == QUEUECAMION)
            SDL_BlitSurface (data->img_camion[1], NULL, data->img_screen, &position);
        else if(data->board_ent[y][x].val == AVION)
          SDL_BlitSurface (data->img_avion, NULL, data->img_screen, &position);
        else if(data->board_ent[y][x].val == TETEBUCHE)
          SDL_BlitSurface (data->img_buche[0], NULL, data->img_screen, &position);
        else if(data->board_ent[y][x].val == MIDBUCHE)
            SDL_BlitSurface (data->img_buche[1], NULL, data->img_screen, &position);
        else if(data->board_ent[y][x].val == QUEUEBUCHE)
            SDL_BlitSurface (data->img_buche[2], NULL, data->img_screen, &position);
        else if(data->board_ent[y][x].val == TORTUE)
            SDL_BlitSurface (data->img_tortue[data->vincentTesUneBranque%13], NULL, data->img_screen, &position);
        else if(data->board_ent[y][x].val == TETECROCO)
            SDL_BlitSurface (data->img_croco[0], NULL, data->img_screen, &position);
        else if(data->board_ent[y][x].val == MIDCROCO)
            SDL_BlitSurface (data->img_croco[1], NULL, data->img_screen, &position);
        else if(data->board_ent[y][x].val == QUEUECROCO)
            SDL_BlitSurface (data->img_croco[2], NULL, data->img_screen, &position);
      }
    }
}

void draw_entity(Data* data, Entity* e, SDL_Surface* s) 
{
    SDL_Rect position;
    position.x = e->x * ELMTSIZE;
    position.y = e->y * ELMTSIZE;
    SDL_BlitSurface (s, NULL, data->img_screen, &position);
} 

void draw_board (Data * data, int x, int y)
{
  SDL_Rect position;
  position.x = x * ELMTSIZE;
  position.y = y * ELMTSIZE;
  if(data->board[y][x] == -1 || data->board[y][x] == EMPTY)
    SDL_BlitSurface (data->img_empty, NULL, data->img_screen, &position);
  else if(data->board[y][x] == WATER)
    SDL_BlitSurface (data->img_water, NULL, data->img_screen, &position);
  else if(data->board[y][x] == STREET)
    SDL_BlitSurface (data->img_street, NULL, data->img_screen, &position);
  else if(data->board[y][x] == HERBE)
    SDL_BlitSurface (data->img_herbe, NULL, data->img_screen, &position);
  else if(data->board[y][x] == NOPE)
    SDL_BlitSurface (data->img_nope, NULL, data->img_screen, &position);
  else if(data->board[y][x] == YEP)
    SDL_BlitSurface (data->img_yep, NULL, data->img_screen, &position);
}

void init_music (Data* data)
{
    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) == -1) 
    {
        printf("%s", Mix_GetError());
    }
    data->save_sound = Mix_LoadWAV("sound/1-up.wav");
    if(!data->save_sound) 
        printf("Mix_LoadWAV: %s\n", Mix_GetError());
    data->frogdie = Mix_LoadWAV("sound/mariodie.wav");
    if(!data->frogdie) 
        printf("Mix_LoadWAV: %s\n", Mix_GetError());
    data->game_over = Mix_LoadWAV("sound/gameover.wav");
    if(!data->game_over) 
        printf("Mix_LoadWAV: %s\n", Mix_GetError());
    data->stage_clear = Mix_LoadWAV("sound/stage_clear.wav");
    if(!data->stage_clear) 
        printf("Mix_LoadWAV: %s\n", Mix_GetError());
    data->bump = Mix_LoadWAV("sound/bump.wav");
    if(!data->bump)
        printf("Mix_LoadWAV: %s\n", Mix_GetError());
    data->etoile = Mix_LoadWAV("sound/etoile.wav");
    if(!data->etoile)
        printf("Mix_LoadWAV: %s\n", Mix_GetError());
    data->music = Mix_LoadMUS("sound/main1.mp3");
    data->music2 = Mix_LoadMUS("sound/main2.mp3");
    data->music3 = Mix_LoadMUS("sound/main3.mp3");
    data->music4 = Mix_LoadMUS("sound/main4.mp3");
    data->music_victoire = Mix_LoadMUS("sound/ff.mp3");
}


void init_game(Data* data)//the function which will initialize every bit of data needed to launch a game
{
  lire(data, LEVEL);
  data->lvl = LEVEL;
  data->victoire=5;
  data->crono = 200;
  data->booleanson = 0;
  data->boulscore = 0;
  data->booleansonfin = 1;
  data->music_vicint = 0;
  data->vincentTesUneBranque=-1;
  data->heroe.x = WIDTH/2;
  data->heroe.y = HEIGHT-3;
  data->heroe.dir = 0;
  draw_entity(data, &data->heroe, data->img_heroe[data->heroe.dir]);
  int x, y, i;
  // refresh of the board[][]
  SDL_FillRect(data->img_screen, NULL, SDL_MapRGB(data->img_screen->format, 0, 0, 0));
  for(x = 0; x < WIDTH; x++) 
  {
    for(y = 0; y < HEIGHT; y++) 
    {
      draw_board(data, x, y);
    }
  }

  //life print
  x=0; SDL_Rect position;
  position.x=0; position.y=(HEIGHT-1) * ELMTSIZE; 
  while(x<data->life)
  {
    SDL_BlitSurface (data->img_life, NULL, data->img_screen, &position);
    position.x+=ELMTSIZE;
    x++;
  }
  //crono print
  x=0;
  position.x=(WIDTH-1) * ELMTSIZE + 20; position.y=(HEIGHT-1) * ELMTSIZE + 7; 
  while(x<data->crono)
  {
    SDL_BlitSurface (data->img_crono, NULL, data->img_screen, &position);
    position.x--;
    x++;
  }

  for(i = 0, x = 0; x < data->img_nombre->w; x+=15, i++)
  {
    data->clip[ i ].x = x;
    data->clip[ i ].y = 0;
    data->clip[ i ].w = 14;
    data->clip[ i ].h = 20;
  }


  //song initialisation
  if(data->niveau == 1)
  {
    if(Mix_PlayMusic(data->music, 1) == -1) 
      printf("Mix_PlayMusic: %s\n", Mix_GetError());
  }
  else if (data->niveau == 2)
  {
    Mix_HaltMusic();
    if(Mix_PlayMusic(data->music2, 1) == -1) 
      printf("Mix_PlayMusic: %s\n", Mix_GetError());
  }
  else if (data->niveau == 3)
  {
    Mix_HaltMusic();
    if(Mix_PlayMusic(data->music3, 1) == -1) 
      printf("Mix_PlayMusic: %s\n", Mix_GetError());
  }
  else if (data->niveau == 4)
  {
    Mix_HaltMusic();
    if(Mix_PlayMusic(data->music4, 1) == -1) 
      printf("Mix_PlayMusic: %s\n", Mix_GetError());
  }

  remplir_board_ent(data);
  board_entity(data);
  gain_point(data, 0, 0);
}

void init_data(Data* data) //the function which will initialize every bit of data needed to launch the game
{
  data->img_screen = SDL_SetVideoMode(WIDTH * ELMTSIZE, HEIGHT * ELMTSIZE, 32, SDL_ANYFORMAT);

  data->img_empty = load("img/empty.png"); 

  data->img_heroe[NORTH] = load("img/greh3.png"); 
  data->img_heroe[WEST] = load("img/greg3.png"); 
  data->img_heroe[SOUTH] = load("img/greb3.png"); 
  data->img_heroe[EAST] = load("img/gred3.png");
  data->heroe.count_down = -1;

  data->img_water = load("img/eau.png");
  data->img_street = load("img/street.png");
  data->img_herbe = load("img/herbe.png");

  data->img_nombre = load("img/nombres.png");
  data->img_lblscore = load("img/lblscore.png");
  data->img_new_partie = load("img/new_partie.png"); 
  data->img_menu = load("img/menu.png"); 

  //Droite
  data->img_life = load("img/greh3.png");
  data->img_nope = load("img/nenu.png");
  data->img_yep = load("img/save.png");
  data->img_victoire = load("img/victoire.png");
  data->img_voiture = load("img/voiture.png");
  data->img_voiture2 = load("img/voiture2.png");
  data->img_voiture3 = load("img/voiture3.png");
  data->img_camion[0] = load("img/tetecamion.png");
  data->img_camion[1] = load("img/queuecamion.png");
  data->img_avion = load("img/avion.png");
  data->img_buche[0] = load("img/tetebuche.png");
  data->img_buche[1] = load("img/midbuche.png");
  data->img_buche[2] = load("img/queuebuche.png");
  
  data->img_tortue[0]= load("img/tortue.png");
  data->img_tortue[1]= load("img/tortue3.png");
  data->img_tortue[2]= data->img_tortue[0];
  data->img_tortue[3]= data->img_tortue[1];
  data->img_tortue[4]= data->img_tortue[0];
  data->img_tortue[5]= data->img_tortue[1];
  data->img_tortue[6]= load("img/tortue4.png");
  data->img_tortue[7]= load("img/tortue5.png");
  data->img_tortue[8]= data->img_water;
  data->img_tortue[9]= data->img_water;
  data->img_tortue[10]= data->img_water;
  data->img_tortue[11]= data->img_tortue[7];
  data->img_tortue[12]= data->img_tortue[6];

  data->img_croco[0] = load("img/tetecroco.png");
  data->img_croco[1] = load("img/midcroco.png");
  data->img_croco[2] = load("img/queuecroco.png");
  data->img_croco2 = load("img/tete2croco.png");

  //Gauche
  data->img_voiture_g = load("imgg/voiture.png");
  data->img_voiture2_g = load("imgg/voiture2.png");
  data->img_voiture3_g = load("imgg/voiture3.png");
  data->img_camion_g[0] = load("imgg/tetecamion.png");
  data->img_camion_g[1] = load("imgg/queuecamion.png");
  data->img_avion_g = load("imgg/avion.png");

  data->img_tortue_g[0]= load("imgg/tortue.png");
  data->img_tortue_g[1]= load("imgg/tortue3.png");
  data->img_tortue_g[2]= data->img_tortue[0];
  data->img_tortue_g[3]= data->img_tortue[1];
  data->img_tortue_g[4]= data->img_tortue[0];
  data->img_tortue_g[5]= data->img_tortue[1];
  data->img_tortue_g[6]= load("imgg/tortue4.png");
  data->img_tortue_g[7]= load("imgg/tortue5.png");
  data->img_tortue_g[8]= data->img_water;
  data->img_tortue_g[9]= data->img_water;
  data->img_tortue_g[10]= data->img_water;
  data->img_tortue_g[11]= data->img_tortue_g[7];
  data->img_tortue_g[12]= data->img_tortue_g[6];

  data->img_croco_g[0] = load("imgg/tetecroco.png");
  data->img_croco_g[1] = load("img/midcroco.png");
  data->img_croco_g[2] = load("imgg/queuecroco.png");
  data->img_croco2_g = load("imgg/tete2croco.png");

  data->img_game_over = load("img/GO.png");
  data->img_crono = load("img/timer.png");
  data->img_crononul = load("img/timernul.png");
  
  data->niveau = 1;
  data->score = 0;
  data->cheatmode = 0;
  data->life = 5;

  init_game(data) ;
}

SDL_Surface * find_good_sdl_img (Data * data, int number, int pos)//
{
    if(number == VOITURE){
      if(pos)
        return data->img_voiture;
      return data->img_voiture_g;
    }
    else if(number == VOITURE2)
    {
      if(pos)
        return data->img_voiture2;
      return data->img_voiture2_g;
    }
    else if(number == TETECAMION)
    {
      if(pos)
        return data->img_camion[0];
      return data->img_camion_g[0];
    }
    else if(number == QUEUECAMION)
    {
      if(pos)
        return data->img_camion[1];
      return data->img_camion_g[1];
    }
    else if(number == AVION)
    {
      if(pos)
        return data->img_avion;
      return data->img_avion_g;
    }
    else if(number == TETEBUCHE)
      return data->img_buche[0];
    else if(number == MIDBUCHE)
        return data->img_buche[1];
    else if(number == QUEUEBUCHE)
        return data->img_buche[2];
    else if(number == TORTUE)
    {
      if(pos)
        return data->img_tortue[data->vincentTesUneBranque%13];
      return data->img_tortue_g[data->vincentTesUneBranque%13];
    }
    else if(number == TETECROCO)
    {
      if(pos){
        if(data->vincentTesUneBranque%2)

          return data->img_croco[0];
        else
          return data->img_croco2;
      }
      else{
        if(data->vincentTesUneBranque%2)

          return data->img_croco_g[0];
        else
          return data->img_croco2_g;
      }
    }
    else if(number == MIDCROCO)
    {
      if(pos)
        return data->img_croco[1];
      return data->img_croco_g[1];
    }
    else
    {
      if(pos)
        return data->img_croco[2];
      return data->img_croco_g[2];
    }
    //return NULL;
}

void draw_at_pos_x_y(Data* data, int x, int y, SDL_Surface* s) 
{
    SDL_Rect position;
    position.x = x * ELMTSIZE;
    position.y = y * ELMTSIZE;
    SDL_BlitSurface (s, NULL, data->img_screen, &position);
}

void death(Data* data)
{
  SDL_Rect position;

  data->heroe.x = WIDTH/2;
  data->heroe.y = HEIGHT-3;
  data->life-- ;

  Mix_PauseMusic();
  if(Mix_PlayChannel(0, data->frogdie, 0) == -1) 
    printf("Mix_PlayChannel: %s\n",Mix_GetError());

  //Efface la derniere vie.
  position.x=data->life * ELMTSIZE; position.y=(HEIGHT-1) * ELMTSIZE; 
  SDL_BlitSurface (data->img_empty, NULL, data->img_screen, &position);
  position.x+=ELMTSIZE;
  //************************

  if(data->life==0)
  {
    data->lvl = GAMEOVER;
  }
  else
  {
    draw_board(data,data->heroe.x,data->heroe.y);
  }
}   

void move_ligne(Data* data, int line, int dir){
  int tmp, x;
  if(dir==WEST){
    tmp = data->board_ent[line][0].val;


    if(data->board_ent[line][0].heroe==1){
      data->board_ent[line][0].heroe=0;
      death(data) ;
    }
    for(x=1;x<WIDTH;x++){

      if (data->board_ent[line][x].val >= 0)
      {
        data->board_ent[line][x+coor[dir].x].val = data->board_ent[line][x].val;
        data->board_ent[line][x].val = 0;
        if(data->board_ent[line][x].heroe==1){
          data->board_ent[line][x].heroe=0;
          data->heroe.x=x+coor[dir].x;
          data->board_ent[line][x+coor[dir].x].heroe=1;
        }
      }
    }
    data->board_ent[line][WIDTH-1].val = tmp ;
    for(x=0;x<WIDTH;x++){
      if(data->board_ent[line][x].val>=0){
        if(data->board[line][x] == WATER)
          draw_at_pos_x_y(data, x, line, data->img_water);
        if(data->board[line][x] == EMPTY)
          draw_at_pos_x_y(data, x, line, data->img_empty);
        if(data->board_ent[line][x].val>0)
          draw_at_pos_x_y(data, x, line, find_good_sdl_img(data, data->board_ent[line][x].val, 0));
      }
    }
  }
  
  else if(dir==EAST){
    tmp = data->board_ent[line][WIDTH-1].val ;

    if(data->board_ent[line][WIDTH-1].heroe==1){
      data->board_ent[line][WIDTH-1].heroe=0;
      death(data);
    }

    for(x=WIDTH-2;x>=0;x--){
      if (data->board_ent[line][x].val >= 0)
      {
        data->board_ent[line][x+coor[dir].x].val = data->board_ent[line][x].val;
        data->board_ent[line][x].val = 0;
        if(data->board_ent[line][x].heroe==1){
          data->board_ent[line][x].heroe=0;
          data->heroe.x=x+coor[dir].x;
          data->board_ent[line][x+coor[dir].x].heroe=1;
        }
      }
    }
    data->board_ent[line][0].val = tmp ;
    for(x=WIDTH-1;x>=0;x--){
      if(data->board_ent[line][x].val>=0){
        if(data->board[line][x] == WATER)
          draw_at_pos_x_y(data, x, line, data->img_water);
        if(data->board[line][x] == EMPTY)
          draw_at_pos_x_y(data, x, line, data->img_empty);
        if(data->board_ent[line][x].val>0)
          draw_at_pos_x_y(data, x, line, find_good_sdl_img(data, data->board_ent[line][x].val, 1));
      }
    }
  }
}


void move_all (Data* data)
{
  int x, y, line;
  data->vincentTesUneBranque++;
  for(y = 0; y < 8; y++) 
    {
      line = line_for_ent[y];
      if(y&1){
        move_ligne(data, line, WEST);
      }
      else{
        move_ligne(data, line, EAST);
      }
    }
    if(data->board_ent[data->heroe.y][data->heroe.x].val >= TETECROCO && data->cheatmode == 0)
      death(data);

    if((data->vincentTesUneBranque%13)>=8 && (data->vincentTesUneBranque%13)<=10 && data->board_ent[data->heroe.y][data->heroe.x].val==TORTUE && data->cheatmode == 0)
    {
      data->board_ent[data->heroe.y][data->heroe.x].heroe=0;
      death(data);
    }

    
}

int valid_pos(Data* data, int x, int y, int dir)
{
  if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT && data->board[y][x] != YEP && data->board[y][x] != -1 && data->board[y][x] != HERBE)
    return 1;
  return 0;
}

void move_entity(Data* data, Entity* e, int dir) //the function which will
{
  int x = e->x + coor[dir].x;
  int y = e->y + coor[dir].y;
  if(data->cheatmode == 1)
  {
    if(data->board[y][x]==NOPE)
      goto CHEATMODE;
    e->x = x;
    e->y = y;
    e->dir = dir;
    return;
  }
  if(dir == -1)
    return;

  if(valid_pos(data, x, y, dir)) 
  {
    if(Mix_PlayChannel(0, data->bump, 0) == -1) 
      printf("Mix_PlayChannel: %s\n",Mix_GetError());
    data->board_ent[e->y][e->x].heroe = 0;
    
    //Si le héros arrive sur une case BUT
    CHEATMODE: // Etiquette pour le cheatmode seulement.
    if(data->board[y][x]==NOPE)
    {
      data->board[y][x]=YEP;
      draw_board(data,x,y);
      data->victoire--;
      if(data->victoire != 0 && Mix_PlayChannel(0, data->save_sound, 0) == -1) 
          printf("Mix_PlayChannel: %s\n",Mix_GetError());
      if(data->victoire==0)
      {
        data->lvl=WIN;
      }
      data->heroe.x = WIDTH/2;
      data->heroe.y = HEIGHT-3;
      data->heroe.dir = 0;

      gain_point(data, 50, 0);
    }
    //Fin de case BUT
    //Si héros est sur tete croco
    else if(data->board_ent[y][x].val>=TETECROCO)
    {
      death(data);
    }
    //Si héros sur l'eau
    else if(data->board[y][x]==WATER)
    {
      //Pas d'entités sur l'eau donc mort.
      if(data->board_ent[y][x].val==0)
      {
        death(data);
      }
      else if(data->board_ent[y][x].val<TETECROCO)
      {
        if(data->vincentTesUneBranque%13>=8 && data->vincentTesUneBranque%13<=10 && data->board_ent[y][x].val==TORTUE)
          death(data);
        else
        {
          data->board_ent[y][x].heroe=1;
          e->x = x ;
          e->y = y ;
          e->dir = dir ;
        }
      }
    } 
    else
    {
      e->x = x;
      e->y = y;
      e->dir = dir;
    }
  }
}

void board(Data* data, int x, int y) 
{
    SDL_Rect position;
    position.x = x * ELMTSIZE;
    position.y = y * ELMTSIZE;
    if(data->board[y][x] == WATER)
      SDL_BlitSurface (data->img_water, NULL, data->img_screen, &position);
    else if(data->board[y][x] == STREET)
      SDL_BlitSurface (data->img_street, NULL, data->img_screen, &position);
    else if(data->board[y][x] == HERBE)
      SDL_BlitSurface (data->img_herbe, NULL, data->img_screen, &position);
    else if(data->board[y][x] == NOPE)
      SDL_BlitSurface (data->img_nope, NULL, data->img_screen, &position);
    else if(data->board[y][x] == YEP)
      SDL_BlitSurface (data->img_yep, NULL, data->img_screen, &position);
}

void print_crono (Data* data)// the function which will print the timer
{
  SDL_Rect position;
  position.x=(WIDTH-1)*ELMTSIZE + 20 - (data->crono); position.y=(HEIGHT-1) * ELMTSIZE + 7;
  SDL_BlitSurface (data->img_crononul, NULL, data->img_screen, &position);
}

void loop_game_render(Data* data) 
{
  //Timer pour le déplacement des entités
  data->currentTime = SDL_GetTicks();
  if (data->currentTime > data->precTime + (800 / (data->niveau * 0.7)))
  {
    move_all(data);
    SDL_Flip(data->img_screen);
    data->precTime = data->currentTime;
    if(data->crono < 0)
      data->lvl = GAMEOVER;
    else
    {
      data->crono--;
      print_crono(data);
    }
  }
  //Timer pour le son principale coupé en cas d'un autre son
  data->currentTime2 = SDL_GetTicks();
  if(data->cheatmode == 0 && Mix_PausedMusic() == 1)
  {
    if(data->booleanson == 0)
    {
      data->precTime2 = SDL_GetTicks();
      data->booleanson = 1;
    }
    if(data->currentTime2 > data->precTime2 + 3500)
    {
      Mix_ResumeMusic();
      data->booleanson = 0;
      data->precTime2 = data->currentTime2;
    }
  }
  //Repeindre après le passage du héros
  if(data->board[data->heroe.y][data->heroe.x] != WATER)
    draw_board(data, data->heroe.x, data->heroe.y);
  else
    draw_at_pos_x_y(data, data->heroe.x, data->heroe.y, find_good_sdl_img(data, data->board_ent[data->heroe.y][data->heroe.x].val, 0));

  SDL_Event event;
  if(SDL_PollEvent(&event)) 
  {
    switch (event.type) 
      {
        case SDL_KEYDOWN:
        switch(event.key.keysym.sym) 
        {
          case SDLK_RIGHT:
            data->heroe.dir = EAST;
            move_entity(data, &data->heroe, data->heroe.dir);
            break;
          case SDLK_LEFT:
            data->heroe.dir = WEST;
            move_entity(data, &data->heroe, data->heroe.dir);
            break;
          case SDLK_UP:
            data->heroe.dir = NORTH;
            move_entity(data, &data->heroe, data->heroe.dir);
            break;
          case SDLK_DOWN:
            data->heroe.dir = SOUTH;
            move_entity(data, &data->heroe, data->heroe.dir);
            break;
          case SDLK_c:
            if (data->cheatmode == 1)
            {
              Mix_ResumeMusic();
              Mix_HaltChannel(-1);
              data->cheatmode = 0;
            }
            else
            {
              Mix_PauseMusic();
              if(Mix_PlayChannel(-1, data->etoile, 0) == -1) 
                printf("Mix_PlayChannel: %s\n",Mix_GetError());
              data->cheatmode = 1;
            }
            break;
          case SDLK_s:
            data->boulscore = 1;
            break;
          default: break;
        }
        break;
        default: break;
      }
  }
  //Peindre si le héros s'est fais mangé sinon peindre le héros
  if(data->board[data->heroe.y][data->heroe.x] != WATER)
    draw_board(data, data->heroe.x, data->heroe.y);
  else
    draw_at_pos_x_y(data, data->heroe.x, data->heroe.y, find_good_sdl_img(data, data->board_ent[data->heroe.y][data->heroe.x].val, 0));

  draw_entity(data, &data->heroe, data->img_heroe[data->heroe.dir]);
}

void loop_game_loose (Data* data)
{
  if(data->booleansonfin == 1)
  {
    Mix_HaltMusic();
    if(Mix_PlayChannel(0, data->game_over, 0) == -1) 
      printf("Mix_PlayChannel: %s\n",Mix_GetError());
    data->booleansonfin = 0;
  }
  SDL_Rect position;
  position.x = (WIDTH/2 - 3) * ELMTSIZE;
  position.y = HEIGHT/2 * ELMTSIZE;
  SDL_BlitSurface (data->img_game_over, NULL, data->img_screen, &position);
  SDL_Flip(data->img_screen);

  SDL_Event event;
  if(SDL_PollEvent(&event)) 
  {
    switch (event.type) 
    {
      case SDL_KEYDOWN:
        switch(event.key.keysym.sym) 
        {
          case SDLK_n:
              Mix_HaltMusic();
              data->cheatmode = 0;
              data->niveau=1;
              init_game(data);
            break;
          default: break;
        }
      default: break;
    }
  }
}

void loop_game_win (Data* data)
{
  SDL_Rect position;
  position.x =  2*ELMTSIZE;
  position.y =  ELMTSIZE;
  SDL_BlitSurface (data->img_victoire, NULL, data->img_screen, &position);
  SDL_Flip(data->img_screen); 
  data->niveau++;
  if(data->boulscore == 1)
    gain_point(data, 500, 1);
  if(data->niveau < 4)
  {
    if(data->boulscore == 0)
      gain_point(data, 500, 1);
    Mix_PauseMusic();
    if(Mix_PlayChannel(0, data->stage_clear, 0) == -1) 
      printf("Mix_PlayChannel: %s\n",Mix_GetError());
    init_game(data);
    return;
  }
  
  if (data->music_vicint == 0)
  {
    Mix_HaltMusic();
    data->music_vicint = 1;
  }
  if(data->music_vicint == 1)
    if(Mix_PlayMusic(data->music_victoire, 1) == -1)
    {
      data->music_vicint = 0;
      printf("Mix_PlayyMusic: %s\n", Mix_GetError());
      data->music_vicint = 2;
    }
}

void renderloop(void* arg) 
{
  Data* data = arg;
  SDL_Rect position;
  SDL_Event event;
  if(data->boulmenu == 0)
  {
  	Mix_PauseMusic();
  	SDL_FillRect(data->img_screen, NULL, SDL_MapRGB(data->img_screen->format, 0, 0, 0));
  	draw_at_pos_x_y(data, 5, 2, data->img_menu);
  	draw_at_pos_x_y(data, 2, 7, data->img_new_partie);
  	if(SDL_PollEvent(&event)) 
  	{
  	  switch (event.type) 
  	  {
  	    case SDL_KEYDOWN:
  	      	switch(event.key.keysym.sym) 
  	      	{
  	        	case SDLK_n:
  	        		data->boulmenu = 1;
  	        		init_music(data);
  					init_data(data);
  					break;
  				default:break;
  			}
  		}
  	}
  }
  if(data->boulmenu == 1)
  {
  	 switch(data->lvl) 
  	{
    	case LEVEL: 
    	  loop_game_render(data);
    	  break;
    	case GAMEOVER:
    	  loop_game_loose(data);
    	  break;
    	  case WIN:
    	  loop_game_win(data);
    	  break;
    	  default: break;
  	}
  }
}

int main ()
{
  srand(time(NULL));
  Data* data = malloc(sizeof *data);
  SDL_Init(SDL_INIT_VIDEO);

  init_music(data);
  init_data(data);
  data->boulmenu = 0;            
  emscripten_set_main_loop_arg(renderloop, data, 0, 0);
  return 0;
}
