#include "ui.h"
#include "crypto.h"
#include "cloud.h"
#include "commands.h"
#include "version.h"
#include "../compat/curses_compat.h"
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>

#ifndef KEY_SLEFT
#define KEY_SLEFT 393
#endif
#ifndef KEY_SRIGHT
#define KEY_SRIGHT 402
#endif

#define UI_INPUT_MAX 4096
#define UI_CMD_HISTORY 48

typedef struct { char text[1024]; bool local; uint64_t ts; } Message;
typedef struct { char id[65]; char filename[256]; char path[BALANCE_MAX_PATH]; uint64_t size; uint64_t done; uint64_t started; uint64_t last_done; uint64_t last_ms; uint8_t digest[32]; int fd; bool receiving; bool finished; bool accepted; bool sender; bool sender_running; pthread_t sender_thread; char tunnel[BALANCE_MAX_TUNNEL]; } Transfer;
typedef struct { char label[128]; char command[256]; int x0; int x1; int y0; int y1; } Action;
typedef enum { VIEW_HOME, VIEW_CHAT } ViewMode;
typedef enum { THEME_CYBER, THEME_MONO, THEME_AMBER } ThemeMode;
typedef struct QNode { NetEvent ev; uint8_t *data; struct QNode *next; } QNode;

typedef struct { App *a; char id[65]; char path[BALANCE_MAX_PATH]; char tunnel[BALANCE_MAX_TUNNEL]; uint64_t size; } SendArgs;

struct App {
    Store *store; Vault *vault; NetContext *net; uint8_t master_key[32];
    WINDOW *w_header,*w_chat,*w_side,*w_actions,*w_input;
    int rows,cols,chat_h,side_w; ViewMode view; ThemeMode theme;
    char current_user[BALANCE_MAX_USERNAME]; char chat_peer[BALANCE_MAX_USERNAME]; char chat_tunnel[BALANCE_MAX_TUNNEL];
    char input[UI_INPUT_MAX]; size_t input_len,input_pos; size_t scroll;
    Message msgs[UI_MAX_MESSAGES]; size_t msg_count;
    Action actions[UI_MAX_ACTIONS]; size_t action_count;
    Transfer transfers[UI_MAX_TRANSFERS];
    char cmd_history[UI_CMD_HISTORY][UI_INPUT_MAX]; size_t cmd_history_count; size_t cmd_history_pos;
    pthread_mutex_t mu; pthread_mutex_t q_mu; QNode *q_head,*q_tail;
    char status[512]; uint64_t status_until; bool running; bool exit_requested; bool colors;
};

static int color_pair(const App*a,int id){if(!a->colors){(void)id;return 0;}return COLOR_PAIR(id);}
static void theme_init(App*a){a->colors=false;if(has_colors()){start_color();use_default_colors();a->colors=true;}a->theme=THEME_CYBER;}
static void theme_apply(App*a){if(!a->colors)return;switch(a->theme){case THEME_AMBER:init_pair(1,COLOR_YELLOW,-1);init_pair(2,COLOR_YELLOW,-1);init_pair(3,COLOR_WHITE,-1);init_pair(4,COLOR_RED,-1);init_pair(5,COLOR_YELLOW,-1);init_pair(6,COLOR_GREEN,-1);break;case THEME_MONO:init_pair(1,COLOR_WHITE,-1);init_pair(2,COLOR_WHITE,-1);init_pair(3,COLOR_WHITE,-1);init_pair(4,COLOR_WHITE,-1);init_pair(5,COLOR_WHITE,-1);init_pair(6,COLOR_WHITE,-1);break;default:init_pair(1,COLOR_CYAN,-1);init_pair(2,COLOR_GREEN,-1);init_pair(3,COLOR_BLUE,-1);init_pair(4,COLOR_RED,-1);init_pair(5,COLOR_YELLOW,-1);init_pair(6,COLOR_MAGENTA,-1);break;}}

static void ui_add_msg(App*a,const char*text,bool local){if(!a||!text)return;pthread_mutex_lock(&a->mu);if(a->msg_count==UI_MAX_MESSAGES){memmove(a->msgs,a->msgs+1U,(UI_MAX_MESSAGES-1U)*sizeof(a->msgs[0]));a->msg_count--;}snprintf(a->msgs[a->msg_count].text,sizeof(a->msgs[a->msg_count].text),"%s",text);a->msgs[a->msg_count].local=local;a->msgs[a->msg_count].ts=now_millis();a->msg_count++;a->scroll=0;pthread_mutex_unlock(&a->mu);}
static void ui_status(App*a,const char*s){pthread_mutex_lock(&a->mu);snprintf(a->status,sizeof(a->status),"%s",s?s:"");a->status_until=now_millis()+7000ULL;pthread_mutex_unlock(&a->mu);}
static void action_add(App*a,const char*label,const char*cmd){if(a->action_count>=UI_MAX_ACTIONS)return;for(size_t i=0;i<a->action_count;++i)if(strcmp(a->actions[i].command,cmd)==0)return;Action*x=&a->actions[a->action_count++];snprintf(x->label,sizeof(x->label),"%s",label);snprintf(x->command,sizeof(x->command),"%s",cmd);}
static void action_remove(App*a,const char*cmd){for(size_t i=0;i<a->action_count;++i)if(strcmp(a->actions[i].command,cmd)==0){memmove(&a->actions[i],&a->actions[i+1U],(a->action_count-i-1U)*sizeof(a->actions[0]));a->action_count--;i--;}}
static Transfer *transfer_find(App*a,const char*id){for(size_t i=0;i<UI_MAX_TRANSFERS;++i)if(a->transfers[i].id[0]&&strcmp(a->transfers[i].id,id)==0)return &a->transfers[i];return NULL;}
static Transfer *transfer_new(App*a,const char*id){Transfer*t=transfer_find(a,id);if(t)return t;for(size_t i=0;i<UI_MAX_TRANSFERS;++i)if(!a->transfers[i].id[0]||a->transfers[i].finished){memset(&a->transfers[i],0,sizeof(a->transfers[i]));snprintf(a->transfers[i].id,sizeof(a->transfers[i].id),"%s",id);a->transfers[i].fd=-1;return &a->transfers[i];}return NULL;}

static void screen_size(int*rows,int*cols){struct winsize ws={0};if(ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws)==0&&ws.ws_row>0&&ws.ws_col>0){*rows=(int)ws.ws_row;*cols=(int)ws.ws_col;return;}*rows=24;*cols=80;}
static void center_line(WINDOW*w,int row,int width,const char*s,int attr){int n=(int)strlen(s);int x=(width-n)/2;if(x<0)x=0;wattron(w,attr);mvwaddnstr(w,row,x,s,width-x);wattroff(w,attr);}

static void redraw_header(App*a){
    werase(a->w_header);
    char mode[32]; snprintf(mode,sizeof(mode),"%s",a->vault->cloud_enabled?"cloud":"volatile");
    char left[256]; snprintf(left,sizeof(left)," balance  %s ",BALANCE_VERSION);
    wattron(a->w_header,A_BOLD|color_pair(a,1)); mvwaddnstr(a->w_header,0,0,left,a->cols-1); wattroff(a->w_header,A_BOLD|color_pair(a,1));
    char right[512];
    if(a->view==VIEW_CHAT) snprintf(right,sizeof(right)," %s  ·  %s  [%s] ",a->current_user,a->chat_peer,mode);
    else snprintf(right,sizeof(right)," %s  [%s] ",a->current_user,mode);
    int rw=(int)strlen(right); if(rw<a->cols){wattron(a->w_header,A_BOLD|color_pair(a,5));mvwaddnstr(a->w_header,0,a->cols-rw,right,rw);wattroff(a->w_header,A_BOLD|color_pair(a,5));}
    wrefresh(a->w_header);
}

static uint64_t memory_peak_bytes(void){
    struct rusage ru={0};
    if(getrusage(RUSAGE_SELF,&ru)!=0)return 0;
#if defined(__APPLE__)
    return (uint64_t)ru.ru_maxrss;
#else
    return (uint64_t)ru.ru_maxrss*1024ULL;
#endif
}
static void format_bytes(uint64_t bytes,char*out,size_t out_sz){static const char*u[]={"B","KiB","MiB","GiB","TiB"};double v=(double)bytes;size_t i=0;while(v>=1024.0&&i<4){v/=1024.0;i++;}snprintf(out,out_sz,"%.1f %s",v,u[i]);}
static void local_time(char*out,size_t n){time_t t=time(NULL);struct tm tmv={0};localtime_r(&t,&tmv);strftime(out,n,"%Y-%m-%d %H:%M:%S",&tmv);}

static void redraw_chat(App*a){
    werase(a->w_chat);
    pthread_mutex_lock(&a->mu);
    int width=a->cols-2; if(width<10)width=10;
    int prefix=25, content=width-prefix; if(content<8)content=8;
    size_t total_lines=0;
    for(size_t i=0;i<a->msg_count;++i){
        const char *p=a->msgs[i].text;
        for(;;){
            const char *nl=strchr(p,'\n');
            size_t logical_len=nl?(size_t)(nl-p):strlen(p);
            size_t parts=logical_len==0?1U:(logical_len+(size_t)content-1U)/(size_t)content;
            total_lines+=parts;
            if(!nl)break;
            p=nl+1;
        }
    }
    size_t max_scroll=total_lines>(size_t)a->chat_h?total_lines-(size_t)a->chat_h:0;
    if(a->scroll>max_scroll)a->scroll=max_scroll;
    size_t first_line=total_lines>(size_t)a->chat_h+a->scroll?total_lines-(size_t)a->chat_h-a->scroll:0;
    size_t line_no=0; int row=0;
    for(size_t i=0;i<a->msg_count&&row<a->chat_h;++i){
        const Message*m=&a->msgs[i];
        char ts[32]; time_t tt=(time_t)(m->ts/1000ULL); struct tm tmv={0}; localtime_r(&tt,&tmv); strftime(ts,sizeof(ts),"%H:%M:%S",&tmv);
        const char*who=m->local?"you":a->chat_peer;
        const char *p=m->text; bool first_segment=true;
        for(;;){
            const char *nl=strchr(p,'\n');
            size_t logical_len=nl?(size_t)(nl-p):strlen(p);
            size_t parts=logical_len==0?1U:(logical_len+(size_t)content-1U)/(size_t)content;
            for(size_t part=0;part<parts && row<a->chat_h;++part){
                if(line_no>=first_line){
                    char line[1200]; size_t off=part*(size_t)content;
                    size_t take=logical_len>off?logical_len-off:0; if(take>(size_t)content)take=(size_t)content;
                    if(first_segment&&part==0)snprintf(line,sizeof(line)," %s  %-10.10s  %.*s",ts,who,(int)take,p+off);
                    else if(part==0)snprintf(line,sizeof(line),"                      %.*s",(int)take,p+off);
                    else snprintf(line,sizeof(line),"                      %.*s",(int)take,p+off);
                    int pair=m->local?2:3; wattron(a->w_chat,color_pair(a,pair));mvwaddnstr(a->w_chat,row++,0,line,a->cols-1);wattroff(a->w_chat,color_pair(a,pair));
                }
                line_no++;
            }
            first_segment=false;
            if(!nl)break;
            p=nl+1;
        }
    }
    pthread_mutex_unlock(&a->mu);
    wrefresh(a->w_chat);
}

static void redraw_side(App*a){
    werase(a->w_side);
    for(int x=0;x<a->side_w;x++){mvwaddch(a->w_side,0,x,'-');mvwaddch(a->w_side,a->chat_h-1,x,'-');}
    for(int y=1;y<a->chat_h-1;y++){mvwaddch(a->w_side,y,0,'|');mvwaddch(a->w_side,y,a->side_w-1,'|');}
    int inner=a->side_w-2; if(inner<12){wrefresh(a->w_side);return;}
    const char*title=a->view==VIEW_CHAT?"SESSION":"FRIENDS"; wattron(a->w_side,A_BOLD|color_pair(a,1));mvwaddnstr(a->w_side,1,2,title,inner-2);wattroff(a->w_side,A_BOLD|color_pair(a,1));
    int y=3;
    if(a->view==VIEW_HOME){
        for(size_t i=0;i<a->vault->friend_count && y<a->chat_h-2;++i){Friend*f=&a->vault->friends[i];char label[128];snprintf(label,sizeof(label)," %-*.*s",inner-5,inner-5,f->username);
            wattron(a->w_side,A_BOLD|color_pair(a,6));mvwaddnstr(a->w_side,y,1,label,inner);wattroff(a->w_side,A_BOLD|color_pair(a,6)); y++;
            char state[64];snprintf(state,sizeof(state),"   %s  %s",f->key_pinned?"trusted":"untrusted",f->endpoint);mvwaddnstr(a->w_side,y++,1,state,inner);
        }
        if(a->vault->friend_count==0)mvwaddnstr(a->w_side,y,2,"no friends yet",inner-2);
        mvwaddnstr(a->w_side,a->chat_h-2,2,"click a friend",inner-2);
    }else{
        char line[256]; snprintf(line,sizeof(line),"peer   %s",a->chat_peer); mvwaddnstr(a->w_side,y++,2,line,inner-2);
        snprintf(line,sizeof(line),"tunnel %s",a->chat_tunnel[0]?a->chat_tunnel:"closed"); mvwaddnstr(a->w_side,y++,2,line,inner-2);
        y++;
        int n=0; for(size_t i=0;i<a->action_count && y<a->chat_h-2;++i){Action*ac=&a->actions[i];int bw=inner-4;if(bw<8)break;ac->x0=a->cols-a->side_w+2;ac->x1=a->cols-3;ac->y0=y;ac->y1=y;wattron(a->w_side,A_REVERSE|A_BOLD|color_pair(a,5));mvwaddnstr(a->w_side,y,2,ac->label,bw);wattroff(a->w_side,A_REVERSE|A_BOLD|color_pair(a,5));y++;n++;}
        if(!n)mvwaddnstr(a->w_side,y,2,"no pending actions",inner-2);
        if(y<a->chat_h-2){mvwaddnstr(a->w_side,a->chat_h-2,2,"click an action",inner-2);}
    }
    wrefresh(a->w_side);
}

static void redraw_actions(App*a){
    werase(a->w_actions);
    char status[512]; pthread_mutex_lock(&a->mu);snprintf(status,sizeof(status),"%s",a->status);bool live=a->status_until>now_millis();pthread_mutex_unlock(&a->mu);
    if(live&&status[0]){wattron(a->w_actions,color_pair(a,5)|A_BOLD);mvwaddnstr(a->w_actions,0,1,status,a->cols-2);wattroff(a->w_actions,color_pair(a,5)|A_BOLD);}
    else mvwaddnstr(a->w_actions,0,1,a->view==VIEW_CHAT?"Enter send  ·  Esc back  ·  ↑/↓ or PgUp/PgDn scroll":"Enter open  ·  ↑/↓ history  ·  Ctrl-L redraw",a->cols-2);
    wrefresh(a->w_actions);
}

static void redraw_input(App*a){
    werase(a->w_input);
    int iw=a->cols; for(int x=0;x<iw;x++){mvwaddch(a->w_input,0,x,'-');mvwaddch(a->w_input,2,x,'-');} for(int y=1;y<2;y++){mvwaddch(a->w_input,y,0,'|');mvwaddch(a->w_input,y,iw-1,'|');}
    char prompt[160];snprintf(prompt,sizeof(prompt)," %s %s ",a->view==VIEW_CHAT?a->chat_peer:a->current_user,a->view==VIEW_CHAT?"›":"$");
    int plen=(int)strlen(prompt),avail=a->cols-plen-3;if(avail<1)avail=1;size_t start=0;if(a->input_pos>(size_t)avail)start=a->input_pos-(size_t)avail;size_t n=a->input_len-start;if(n>(size_t)avail)n=(size_t)avail;
    wattron(a->w_input,A_BOLD|color_pair(a,1));mvwaddnstr(a->w_input,1,1,prompt,a->cols-3);wattroff(a->w_input,A_BOLD|color_pair(a,1));
    if(n){mvwaddnstr(a->w_input,1,1+plen,a->input+start,(int)n);}
wmove(a->w_input,1,1+plen+(int)(a->input_pos-start));wrefresh(a->w_input);
}

static void layout(App*a){
    screen_size(&a->rows,&a->cols); a->side_w=a->cols>=100?28:0; a->chat_h=a->rows-5;if(a->chat_h<5)a->chat_h=5;
    if(a->w_header) delwin(a->w_header);
    if(a->w_chat) delwin(a->w_chat);
    if(a->w_side) delwin(a->w_side);
    if(a->w_actions) delwin(a->w_actions);
    if(a->w_input) delwin(a->w_input);
    a->w_header=newwin(1,a->cols,0,0);a->w_chat=newwin(a->chat_h,a->cols,1,0);
    if(a->side_w>0)a->w_side=newwin(a->chat_h,a->side_w,1,a->cols-a->side_w);
    a->w_actions=newwin(1,a->cols,a->rows-4,0);a->w_input=newwin(3,a->cols,a->rows-3,0);
    keypad(a->w_input,TRUE);scrollok(a->w_chat,FALSE);theme_apply(a);
    redraw_header(a);redraw_chat(a);if(a->w_side)redraw_side(a);redraw_actions(a);redraw_input(a);
}

static void fill_command(App*a,const char*cmd){size_t n=strlen(cmd);if(n>=sizeof(a->input))return;memcpy(a->input,cmd,n);a->input[n]='\0';a->input_len=n;a->input_pos=n;}
static void input_insert(App*a,int ch){if(a->input_len+1U>=sizeof(a->input))return;memmove(a->input+a->input_pos+1U,a->input+a->input_pos,a->input_len-a->input_pos);a->input[a->input_pos]=(char)ch;a->input_len++;a->input_pos++;a->input[a->input_len]='\0';}
static void input_back(App*a){if(a->input_pos==0)return;memmove(a->input+a->input_pos-1U,a->input+a->input_pos,a->input_len-a->input_pos);a->input_len--;a->input_pos--;a->input[a->input_len]='\0';}
static void input_del(App*a){if(a->input_pos>=a->input_len)return;memmove(a->input+a->input_pos,a->input+a->input_pos+1U,a->input_len-a->input_pos-1U);a->input_len--;a->input[a->input_len]='\0';}
static void remember_command(App*a,const char*line){if(!line||!*line)return;if(a->cmd_history_count>0&&strcmp(a->cmd_history[a->cmd_history_count-1U],line)==0){a->cmd_history_pos=a->cmd_history_count;return;}if(a->cmd_history_count==UI_CMD_HISTORY){memmove(a->cmd_history,a->cmd_history+1U,(UI_CMD_HISTORY-1U)*sizeof(a->cmd_history[0]));a->cmd_history_count--;}snprintf(a->cmd_history[a->cmd_history_count++],UI_INPUT_MAX,"%s",line);a->cmd_history_pos=a->cmd_history_count;}
static void history_nav(App*a,int dir){if(a->cmd_history_count==0)return;if(dir<0){if(a->cmd_history_pos>0)a->cmd_history_pos--;}else{if(a->cmd_history_pos<a->cmd_history_count)a->cmd_history_pos++;}if(a->cmd_history_pos<a->cmd_history_count)fill_command(a,a->cmd_history[a->cmd_history_pos]);else fill_command(a,"");}

static bool friend_name_prefix(App*a,const char*arg,size_t n,char*out,size_t out_sz){for(size_t i=0;i<a->vault->friend_count;++i)if(strncasecmp(a->vault->friends[i].username,arg,n)==0){snprintf(out,out_sz,"%s",a->vault->friends[i].username);return true;}return false;}
static bool complete_prefix(App*a){size_t n=a->input_pos;if(n<1||a->input[0]!='?')return false;n--;size_t start=0;while(start<n&&!isspace((unsigned char)a->input[start]))start++;char token[256];size_t token_len=start<sizeof(token)-1?start:sizeof(token)-1;memcpy(token,a->input,token_len);token[token_len]='\0';if(start==n){size_t count=0;const BalanceCommandSpec*specs=balance_command_specs(&count);for(size_t i=0;i<count;++i)if(strncasecmp(specs[i].name,token,token_len)==0){fill_command(a,specs[i].name);return true;}for(size_t i=0;i<count;++i)if(specs[i].alias&&strncasecmp(specs[i].alias,token,token_len)==0){fill_command(a,specs[i].alias);return true;}return false;}size_t pos=start;while(pos<n&&isspace((unsigned char)a->input[pos]))pos++;char arg[256];snprintf(arg,sizeof(arg),"%s",a->input+pos);const BalanceCommandSpec*spec=balance_command_lookup(token);if(!spec)return false;if(strcmp(spec->name,"tunnel")==0||strcmp(spec->name,"chat")==0||strcmp(spec->name,"friend")==0||strcmp(spec->name,"add")==0||strcmp(spec->name,"remove")==0||strcmp(spec->name,"ping")==0){char name[256];if(friend_name_prefix(a,arg,strlen(arg),name,sizeof(name))){char cmd[512];snprintf(cmd,sizeof(cmd),"%s %s",spec->name,name);fill_command(a,cmd);return true;}}if(strcmp(spec->name,"accept")==0||strcmp(spec->name,"reject")==0){for(size_t i=0;i<a->action_count;++i){const char*sp=strchr(a->actions[i].command,' ');if(sp&&strncasecmp(sp+1,arg,strlen(arg))==0){fill_command(a,a->actions[i].command);return true;}}}return false;}

static int split_cmd(char*line,char**argv,int max){int n=0;char*save=NULL;char*t=strtok_r(line," \t",&save);while(t&&n<max){argv[n++]=t;t=strtok_r(NULL," \t",&save);}return n;}

static const BalanceCommandSpec *cmdspec(const char *name){return balance_command_lookup(name);}
static void help_group(App*a,const char*title,const char*const*names,size_t n){ui_add_msg(a,title,false);for(size_t i=0;i<n;++i){const BalanceCommandSpec*s=cmdspec(names[i]);if(!s)continue;char buf[768];if(s->alias)snprintf(buf,sizeof(buf),"  %-24s %-32s  [%s]",s->usage,s->description,s->alias);else snprintf(buf,sizeof(buf),"  %-24s %s",s->usage,s->description);ui_add_msg(a,buf,false);}}
static void help(App*a,const char*query){size_t count=0;(void)balance_command_specs(&count);if(query&&*query){const BalanceCommandSpec*s=balance_command_lookup(query);if(!s){char buf[256];snprintf(buf,sizeof(buf),"no such command: %s · try ?",query);ui_add_msg(a,buf,false);return;}char buf[512];snprintf(buf,sizeof(buf),"%s\nusage  %s\nabout  %s",s->name,s->usage,s->description);ui_add_msg(a,buf,false);if(s->alias){snprintf(buf,sizeof(buf),"alias  %s",s->alias);ui_add_msg(a,buf,false);}if(strcmp(s->name,"tunnel")==0)ui_add_msg(a,"example  tunnel alice",false);else if(strcmp(s->name,"file")==0)ui_add_msg(a,"example  file ~/Documents/report.pdf",false);else if(strcmp(s->name,"cloud")==0)ui_add_msg(a,"example  cloud on https://server.example token",false);return;}
ui_add_msg(a,"balance command guide · beta-0.3.1 · ? <command> for details",false);
static const char*core[]={"?","version","about","status","whoami","endpoint","key","memory","net","time"};
static const char*people[]={"friends","online","friend","add","remove","ping"};
static const char*tunnels[]={"tunnel","chat","close","accept","reject"};
static const char*chat[]={"history","search","transfers","file","clear"};
static const char*cloud[]={"cloud","sync","pull","config","doctor"};
static const char*ui[]={"theme","redraw","quit","exit"};
help_group(a,"CORE",core,sizeof(core)/sizeof(core[0]));help_group(a,"PEOPLE",people,sizeof(people)/sizeof(people[0]));help_group(a,"TUNNELS",tunnels,sizeof(tunnels)/sizeof(tunnels[0]));help_group(a,"CHAT",chat,sizeof(chat)/sizeof(chat[0]));help_group(a,"CLOUD",cloud,sizeof(cloud)/sizeof(cloud[0]));help_group(a,"UI",ui,sizeof(ui)/sizeof(ui[0]));
char buf[512];snprintf(buf,sizeof(buf),"%zu commands · ↑/↓ history · PgUp/PgDn scroll · Ctrl-L redraw",count);ui_add_msg(a,buf,false);ui_add_msg(a,"mouse: click friends or pending actions · in chat, normal text sends",false);}


static int make_transfer_id(char out[65]){uint8_t r[16];if(crypto_random(r,sizeof(r))!=BALANCE_OK)return BALANCE_ERR;return crypto_hex(r,sizeof(r),out,65U);}
static void *send_file_worker(void*arg){SendArgs*sa=arg;App*a=sa->a;int fd=open(sa->path,O_RDONLY);if(fd<0){ui_status(a,"file open failed");free(sa);return NULL;}uint8_t buf[BALANCE_FILE_CHUNK];uint64_t sent=0,start=now_millis(),last_report=start;int rc=BALANCE_OK;while(sent<sa->size){size_t want=(sa->size-sent)>BALANCE_FILE_CHUNK?BALANCE_FILE_CHUNK:(size_t)(sa->size-sent);ssize_t nr=read(fd,buf,want);if(nr<=0){rc=BALANCE_ERR;break;}if(net_send_file_chunk(a->net,sa->id,sent,buf,(size_t)nr)!=BALANCE_OK){rc=BALANCE_ERR;break;}sent+=(uint64_t)nr;uint64_t now=now_millis();if(now-last_report>=100U||sent==sa->size){char bar[128],spd[64],st[256];format_progress(sent,sa->size,bar,sizeof(bar));uint64_t elapsed=now>start?now-start:1ULL;format_speed((sent*1000ULL)/elapsed,spd,sizeof(spd));snprintf(st,sizeof(st),"send %s %s · %s",strrchr(sa->path,'/')?strrchr(sa->path,'/')+1:sa->path,bar,spd);ui_status(a,st);last_report=now;}}close(fd);if(rc==BALANCE_OK&&net_send_file_done(a->net,sa->id)==BALANCE_OK)ui_status(a,"file sent and encrypted transfer closed");else ui_status(a,"file transfer failed");free(sa);return NULL;}
static int file_digest(const char*path,uint8_t digest[32],uint64_t*size){int fd=open(path,O_RDONLY);if(fd<0)return BALANCE_ERR;struct stat st;if(fstat(fd,&st)!=0||st.st_size<0||(uint64_t)st.st_size>BALANCE_MAX_FILE){close(fd);return BALANCE_ERR;}EVP_MD_CTX*md=EVP_MD_CTX_new();if(!md){close(fd);return BALANCE_ERR;}if(EVP_DigestInit_ex(md,EVP_sha256(),NULL)!=1){EVP_MD_CTX_free(md);close(fd);return BALANCE_ERR;}uint8_t buf[65536];ssize_t nr;while((nr=read(fd,buf,sizeof(buf)))>0)if(EVP_DigestUpdate(md,buf,(size_t)nr)!=1){EVP_MD_CTX_free(md);close(fd);return BALANCE_ERR;}if(nr<0){EVP_MD_CTX_free(md);close(fd);return BALANCE_ERR;}unsigned int n=32;int ok=EVP_DigestFinal_ex(md,digest,&n)==1;EVP_MD_CTX_free(md);close(fd);*size=(uint64_t)st.st_size;return ok?BALANCE_OK:BALANCE_ERR;}
static int make_volatile_label(char out[BALANCE_MAX_TUNNEL]){static const char*a[]={"ember","quiet","silver","amber","moss","lunar","velvet","cinder","blue","hidden","soft","northern"};static const char*b[]={"lantern","harbor","meadow","signal","river","orbit","garden","bridge","echo","cove","pine","cloud"};uint8_t r[4];if(crypto_random(r,sizeof(r))!=BALANCE_OK)return BALANCE_ERR;int n=snprintf(out,BALANCE_MAX_TUNNEL,"vlt-%s-%s-%02x%02x",a[r[0]%(sizeof(a)/sizeof(a[0]))],b[r[1]%(sizeof(b)/sizeof(b[0]))],r[2],r[3]);return n>0&&n<BALANCE_MAX_TUNNEL?BALANCE_OK:BALANCE_ERR;}

static void load_chat_history(App*a,const char*peer){
    if(!a->vault->cloud_enabled)return;
    HistoryEntry*hist=calloc(MAX_HISTORY,sizeof(*hist));
    if(!hist)return;
    size_t n=0;
    if(cloud_get_history(a->vault,a->store)!=BALANCE_OK){free(hist);return;}
    if(store_load_history(a->store,a->master_key,hist,MAX_HISTORY,&n)!=BALANCE_OK){free(hist);return;}
    char chat[128];snprintf(chat,sizeof(chat),"%s <-> %s",a->current_user,peer);
    for(size_t i=0;i<n;++i)if(strcmp(hist[i].chat,chat)==0)ui_add_msg(a,hist[i].text,hist[i].local);
    secure_zero(hist,MAX_HISTORY*sizeof(*hist));free(hist);
}
static int final_transfer_path(const char*dir,const char*filename,const char*part,char*out,size_t out_sz){char safe[256];if(sanitize_component(filename,safe,sizeof(safe))!=BALANCE_OK)return BALANCE_ERR;const char*dot=strrchr(safe,'.');size_t stem_len=dot&&dot!=safe?(size_t)(dot-safe):strlen(safe);const char*ext=dot&&dot!=safe?dot:"";for(unsigned i=0;i<10000U;++i){int n;if(i==0)n=snprintf(out,out_sz,"%s/%s",dir,safe);else n=snprintf(out,out_sz,"%s/%.*s (%u)%s",dir,(int)stem_len,safe,i,ext);if(n<=0||(size_t)n>=out_sz)return BALANCE_ERR;if(link(part,out)==0){if(unlink(part)!=0){unlink(out);return BALANCE_ERR;}chmod(out,0600);return BALANCE_OK;}if(errno!=EEXIST)return BALANCE_ERR;}return BALANCE_ERR;}

static void clear_view(App*a){pthread_mutex_lock(&a->mu);a->msg_count=0;a->scroll=0;a->status[0]='\0';a->status_until=0;pthread_mutex_unlock(&a->mu);}
static void clear_volatile_chat(App*a){if(!a)return;for(size_t i=0;i<UI_MAX_TRANSFERS;++i){Transfer*t=&a->transfers[i];if(!t->id[0])continue;if(t->fd>=0){close(t->fd);t->fd=-1;}if(t->receiving&&t->path[0]&&!t->finished)(void)unlink(t->path);memset(t,0,sizeof(*t));t->fd=-1;}a->msg_count=0;a->scroll=0;a->chat_tunnel[0]='\0';a->chat_peer[0]='\0';a->action_count=0;}

static void list_friends(App*a,bool probe){size_t online=0;for(size_t i=0;i<a->vault->friend_count;++i){Friend*f=&a->vault->friends[i];if(probe){uint64_t start=now_millis();int rc=net_online_probe(a->net,f);uint64_t ms=now_millis()-start;char line[512];snprintf(line,sizeof(line),"%s  %s  %llums  %s",f->username,rc==BALANCE_OK?"online":"offline",(unsigned long long)ms,f->key_pinned?"trusted":"untrusted");ui_add_msg(a,line,false);if(rc==BALANCE_OK)online++;}else{char fp[65];crypto_hex(f->ed_pub,32U,fp,sizeof(fp));char line[512];snprintf(line,sizeof(line),"%-16s %-24s %s",f->username,f->endpoint,f->key_pinned?fp:"not pinned");ui_add_msg(a,line,false);}}if(probe){char st[128];snprintf(st,sizeof(st),"presence complete · %zu/%zu online",online,a->vault->friend_count);ui_status(a,st);}}

static bool contains_case(const char *hay,const char *needle){
    if(!hay||!needle||!*needle)return false;
    size_t nl=strlen(needle);
    for(const char*p=hay;*p;p++)if(strncasecmp(p,needle,nl)==0)return true;
    return false;
}

static int command_exec(App*a,const char*line){char buf[UI_INPUT_MAX];snprintf(buf,sizeof(buf),"%s",line?line:"");trim_inplace(buf);if(!buf[0])return BALANCE_OK;if(buf[0]!='?'){ui_status(a,"messages are plain text; commands must begin with ?");return BALANCE_ERR;}memmove(buf,buf+1,strlen(buf));trim_inplace(buf);if(!buf[0]){help(a,NULL);return BALANCE_OK;}if(strcmp(buf,"-h")==0){help(a,NULL);return BALANCE_OK;}char first[64]={0};size_t fl=0;while(buf[fl]&&!isspace((unsigned char)buf[fl])&&fl+1<sizeof(first)){first[fl]=buf[fl];fl++;}first[fl]='\0';const BalanceCommandSpec*spec=balance_command_lookup(first);if(!spec){if(a->view==VIEW_CHAT){if(net_send_chat(a->net,a->chat_tunnel,buf)==BALANCE_OK){ui_add_msg(a,buf,true);if(a->vault->cloud_enabled){HistoryEntry he={.ts=now_millis(),.local=true};snprintf(he.chat,sizeof(he.chat),"%s <-> %s",a->current_user,a->chat_peer);snprintf(he.text,sizeof(he.text),"%s",buf);(void)store_append_history(a->store,a->master_key,&he);(void)cloud_put_history(a->vault,a->store);}}else ui_status(a,"send failed");return BALANCE_OK;}ui_status(a,"unknown command · ? for help");return BALANCE_ERR;}if(spec->scope==BALANCE_SCOPE_HOME&&a->view==VIEW_CHAT){ui_status(a,"command is available from home · close the tunnel first");return BALANCE_ERR;}if(spec->scope==BALANCE_SCOPE_CHAT&&a->view!=VIEW_CHAT){ui_status(a,"command is available inside a chat");return BALANCE_ERR;}
    char *argv[12]={0};int argc=split_cmd(buf,argv,12);const char*name=spec->name;
    if(strcmp(name,"?")==0){help(a,argc>1?argv[1]:NULL);return BALANCE_OK;}
    if(argc>1 && strcmp(argv[1],"-h")==0){help(a,name);return BALANCE_OK;}
    if(strcmp(name,"version")==0){char m[256];snprintf(m,sizeof(m),"balance %s · C17 · OpenSSL 3 · curses",BALANCE_VERSION);ui_add_msg(a,m,false);return BALANCE_OK;}
    if(strcmp(name,"about")==0){ui_add_msg(a,"balance is a privacy-first terminal messenger: local encrypted vault, authenticated P2P tunnels, encrypted file transfer, optional encrypted cloud persistence.",false);ui_add_msg(a,"default: volatile mode · no cloud contact · no telemetry · network metadata is not anonymous.",false);return BALANCE_OK;}
    if(strcmp(name,"status")==0){NetStats ns={0};net_get_stats(a->net,&ns);char m[512];snprintf(m,sizeof(m),"mode=%s · view=%s · friends=%zu · peers=%llu · tunnels=%llu · cloud=%s",a->vault->cloud_enabled?"cloud":"volatile",a->view==VIEW_CHAT?"chat":"home",a->vault->friend_count,(unsigned long long)ns.active_peers,(unsigned long long)ns.active_tunnels,a->vault->cloud_enabled?"on":"off");ui_add_msg(a,m,false);return BALANCE_OK;}
    if(strcmp(name,"whoami")==0){ui_add_msg(a,a->current_user,false);return BALANCE_OK;}
    if(strcmp(name,"endpoint")==0){char m[256];snprintf(m,sizeof(m),"listen %s:%u",a->vault->endpoint[0]?a->vault->endpoint:"0.0.0.0",(unsigned)net_listen_port(a->net));ui_add_msg(a,m,false);return BALANCE_OK;}
    if(strcmp(name,"key")==0){char fp[65];crypto_hex(a->vault->ed_pub,32U,fp,sizeof(fp));char m[256];snprintf(m,sizeof(m),"Ed25519 fingerprint %s",fp);ui_add_msg(a,m,false);return BALANCE_OK;}
    if(strcmp(name,"memory")==0){char b[64],m[256];format_bytes(memory_peak_bytes(),b,sizeof(b));snprintf(m,sizeof(m),"process peak RSS %s",b);ui_add_msg(a,m,false);return BALANCE_OK;}
    if(strcmp(name,"net")==0){NetStats ns={0};net_get_stats(a->net,&ns);char tx[64],rx[64],t[512];format_speed(ns.tx_bytes_per_sec,tx,sizeof(tx));format_speed(ns.rx_bytes_per_sec,rx,sizeof(rx));snprintf(t,sizeof(t),"TX %s · RX %s · TX frames %llu · RX frames %llu · peers %llu · tunnels %llu",tx,rx,(unsigned long long)ns.tx_frames,(unsigned long long)ns.rx_frames,(unsigned long long)ns.active_peers,(unsigned long long)ns.active_tunnels);ui_add_msg(a,t,false);return BALANCE_OK;}
    if(strcmp(name,"time")==0){char t[64];local_time(t,sizeof(t));ui_add_msg(a,t,false);return BALANCE_OK;}
    if(strcmp(name,"friends")==0){list_friends(a,argc>1&&strcmp(argv[1],"online")==0);return BALANCE_OK;}
    if(strcmp(name,"online")==0){list_friends(a,true);return BALANCE_OK;}
    if(strcmp(name,"friend")==0){if(argc<2){ui_add_msg(a,"usage: friend <name>",false);return BALANCE_ERR;}Friend*f=NULL;for(size_t i=0;i<a->vault->friend_count;++i)if(strcmp(a->vault->friends[i].username,argv[1])==0)f=&a->vault->friends[i];if(!f){ui_status(a,"unknown friend");return BALANCE_ERR;}char fp[65];crypto_hex(f->ed_pub,32U,fp,sizeof(fp));char m[768];snprintf(m,sizeof(m),"%s · %s · %s · %s",f->username,f->endpoint,f->key_pinned?"trusted":"not pinned",fp);ui_add_msg(a,m,false);return BALANCE_OK;}
    if(strcmp(name,"add")==0){if(argc<3){ui_add_msg(a,"usage: add <name> <host:port>",false);return BALANCE_ERR;}Friend*f=NULL;for(size_t i=0;i<a->vault->friend_count;++i)if(strcmp(a->vault->friends[i].username,argv[1])==0)f=&a->vault->friends[i];if(!f){if(a->vault->friend_count>=MAX_FRIENDS){ui_status(a,"friend list full");return BALANCE_ERR;}f=&a->vault->friends[a->vault->friend_count++];memset(f,0,sizeof(*f));snprintf(f->username,sizeof(f->username),"%s",argv[1]);}snprintf(f->endpoint,sizeof(f->endpoint),"%s",argv[2]);if(store_save_vault_key(a->store,a->vault,a->master_key)!=BALANCE_OK)return BALANCE_ERR;ui_status(a,"friend [##########----------] 50% · sending request");int rc=net_request_friend(a->net,f);ui_status(a,rc==BALANCE_OK?"friend [####################] 100% · request sent":"friend [##------------------] 0% · connection failed");return rc;}
    if(strcmp(name,"remove")==0){if(argc<2){ui_add_msg(a,"usage: remove <name>",false);return BALANCE_ERR;}for(size_t i=0;i<a->vault->friend_count;++i)if(strcmp(a->vault->friends[i].username,argv[1])==0){memmove(&a->vault->friends[i],&a->vault->friends[i+1U],(a->vault->friend_count-i-1U)*sizeof(a->vault->friends[0]));a->vault->friend_count--;if(store_save_vault_key(a->store,a->vault,a->master_key)==BALANCE_OK){ui_status(a,"friend removed");return BALANCE_OK;}return BALANCE_ERR;}ui_status(a,"unknown friend");return BALANCE_ERR;}
    if(strcmp(name,"tunnel")==0){if(argc<2){ui_add_msg(a,"usage: tunnel <friend>",false);return BALANCE_ERR;}Friend*f=NULL;for(size_t i=0;i<a->vault->friend_count;++i)if(strcmp(a->vault->friends[i].username,argv[1])==0)f=&a->vault->friends[i];if(!f||!f->key_pinned){ui_status(a,"friend missing or not trusted");return BALANCE_ERR;}if(make_volatile_label(a->chat_tunnel)!=BALANCE_OK)return BALANCE_ERR;return net_tunnel(a->net,f,a->chat_tunnel);}
    if(strcmp(name,"chat")==0){if(argc<2){ui_add_msg(a,"usage: chat <friend>",false);return BALANCE_ERR;}for(size_t i=0;i<a->vault->friend_count;++i)if(strcmp(a->vault->friends[i].username,argv[1])==0){char peer[128]={0};if(a->chat_tunnel[0]&&net_find_tunnel(a->net,a->chat_tunnel,peer,sizeof(peer))==BALANCE_OK&&strcmp(peer,argv[1])==0){snprintf(a->chat_peer,sizeof(a->chat_peer),"%s",argv[1]);a->view=VIEW_CHAT;load_chat_history(a,argv[1]);ui_add_msg(a,"chat ready · volatile transcript stays in memory",false);return BALANCE_OK;}snprintf(a->chat_peer,sizeof(a->chat_peer),"%s",argv[1]);char tunnel_cmd[256];snprintf(tunnel_cmd,sizeof(tunnel_cmd),"?tunnel %s",argv[1]);return command_exec(a,tunnel_cmd); } ui_status(a,"unknown friend");return BALANCE_ERR;}
    if(strcmp(name,"close")==0){if(!a->chat_tunnel[0]){ui_status(a,"no active tunnel");return BALANCE_ERR;}net_close_tunnel(a->net,a->chat_tunnel);clear_volatile_chat(a);a->view=VIEW_HOME;ui_status(a,"tunnel closed · volatile chat cleared");return BALANCE_OK;}
    if(strcmp(name,"accept")==0){if(argc<2){ui_add_msg(a,"usage: accept <id-or-name>",false);return BALANCE_ERR;}int rc=net_accept_tunnel(a->net,argv[1]);if(rc==BALANCE_OK){snprintf(a->chat_tunnel,sizeof(a->chat_tunnel),"%s",argv[1]);action_remove(a,buf);return BALANCE_OK;}rc=net_accept_file(a->net,argv[1]);if(rc==BALANCE_OK){Transfer*t=transfer_find(a,argv[1]);if(t)t->accepted=true;action_remove(a,buf);return BALANCE_OK;}rc=net_accept_friend(a->net,argv[1]);if(rc==BALANCE_OK){for(size_t i=0;i<a->vault->friend_count;++i)if(strcmp(a->vault->friends[i].username,argv[1])==0){a->vault->friends[i].key_pinned=true;store_save_vault_key(a->store,a->vault,a->master_key);break;}action_remove(a,buf);return BALANCE_OK;}ui_status(a,"no matching request");return BALANCE_ERR;}
    if(strcmp(name,"reject")==0){if(argc<2){ui_add_msg(a,"usage: reject <id> [reason]",false);return BALANCE_ERR;}const char*reason=argc>2?argv[2]:"rejected";int rc=net_reject_tunnel(a->net,argv[1],reason);if(rc==BALANCE_OK){action_remove(a,buf);return BALANCE_OK;}rc=net_reject_file(a->net,argv[1],reason);if(rc==BALANCE_OK){Transfer*t=transfer_find(a,argv[1]);if(t&&t->receiving&&t->path[0])unlink(t->path);action_remove(a,buf);return BALANCE_OK;}rc=net_reject_friend(a->net,argv[1]);if(rc==BALANCE_OK){for(size_t i=0;i<a->vault->friend_count;++i)if(strcmp(a->vault->friends[i].username,argv[1])==0&&!a->vault->friends[i].key_pinned){memmove(&a->vault->friends[i],&a->vault->friends[i+1U],(a->vault->friend_count-i-1U)*sizeof(a->vault->friends[0]));a->vault->friend_count--;store_save_vault_key(a->store,a->vault,a->master_key);break;}action_remove(a,buf);return BALANCE_OK;}ui_status(a,"no matching request");return BALANCE_ERR;}
    if(strcmp(name,"ping")==0){if(argc<2){ui_add_msg(a,"usage: ping <friend>",false);return BALANCE_ERR;}Friend*f=NULL;for(size_t i=0;i<a->vault->friend_count;++i)if(strcmp(a->vault->friends[i].username,argv[1])==0)f=&a->vault->friends[i];if(!f)return BALANCE_ERR;uint64_t st=now_millis();int rc=net_online_probe(a->net,f);uint64_t ms=now_millis()-st;char m[256];snprintf(m,sizeof(m),"ping %s · %s · %llums",f->username,rc==BALANCE_OK?"reachable":"offline",(unsigned long long)ms);ui_add_msg(a,m,false);return rc;}
    if(strcmp(name,"history")==0){
        if(argc>1&&strcmp(argv[1],"clear")==0){if(store_clear_history(a->store)==BALANCE_OK){ui_status(a,"encrypted local history cleared");if(a->view==VIEW_CHAT)clear_view(a);return BALANCE_OK;}return BALANCE_ERR;}
        HistoryEntry*h=calloc(MAX_HISTORY,sizeof(*h));if(!h){ui_status(a,"history allocation failed");return BALANCE_ERR;}size_t n=0;if(store_load_history(a->store,a->master_key,h,MAX_HISTORY,&n)!=BALANCE_OK){free(h);ui_status(a,"no readable local history");return BALANCE_ERR;}
        const char*filter=argc>1?argv[1]:NULL;for(size_t i=0;i<n;++i){if(filter&&strstr(h[i].chat,filter)==NULL)continue;char m[1024];snprintf(m,sizeof(m),"%llu  %s  %s",(unsigned long long)h[i].ts,h[i].chat,h[i].text);ui_add_msg(a,m,h[i].local);}if(!n)ui_add_msg(a,"no local history",false);secure_zero(h,MAX_HISTORY*sizeof(*h));free(h);return BALANCE_OK;}
    if(strcmp(name,"search")==0){const char*needle=argc>1?strstr(buf,argv[1]):NULL;if(!needle){ui_add_msg(a,"usage: search <text>",false);return BALANCE_ERR;}char q[512];snprintf(q,sizeof(q),"%s",needle);trim_inplace(q);size_t hits=0;for(size_t i=0;i<a->msg_count;++i)if(contains_case(a->msgs[i].text,q)){char m[1100];snprintf(m,sizeof(m),"%s %s",a->msgs[i].local?"you:":a->chat_peer,a->msgs[i].text);ui_add_msg(a,m,false);hits++;}char m[128];snprintf(m,sizeof(m),"search complete · %zu hit(s)",hits);ui_status(a,m);return BALANCE_OK;}
    if(strcmp(name,"transfers")==0){size_t shown=0;for(size_t i=0;i<UI_MAX_TRANSFERS;++i)if(a->transfers[i].id[0]){char bar[128],m[512],spd[64];format_progress(a->transfers[i].done,a->transfers[i].size,bar,sizeof(bar));uint64_t el=now_millis()>a->transfers[i].started?now_millis()-a->transfers[i].started:1ULL;format_speed((a->transfers[i].done*1000ULL)/el,spd,sizeof(spd));snprintf(m,sizeof(m),"%s · %s · %s · %s",a->transfers[i].id,a->transfers[i].filename,bar,spd);ui_add_msg(a,m,false);shown++;}if(!shown)ui_add_msg(a,"no active/recent transfers",false);return BALANCE_OK;}
    if(strcmp(name,"file")==0){if(a->view!=VIEW_CHAT||argc<2){ui_add_msg(a,"usage: file <path>",false);return BALANCE_ERR;}const char*p=strstr(buf,argv[0]);p+=strlen(argv[0]);while(*p==' ')p++;if(!*p)return BALANCE_ERR;Transfer*t=NULL;for(size_t i=0;i<UI_MAX_TRANSFERS;++i)if(!a->transfers[i].id[0]||a->transfers[i].finished){t=&a->transfers[i];break;}if(!t)return BALANCE_ERR;uint8_t dig[32];uint64_t fsize=0;if(file_digest(p,dig,&fsize)!=BALANCE_OK){ui_status(a,"cannot read file");return BALANCE_ERR;}char id[65];if(make_transfer_id(id)!=BALANCE_OK)return BALANCE_ERR;memset(t,0,sizeof(*t));snprintf(t->id,sizeof(t->id),"%s",id);snprintf(t->filename,sizeof(t->filename),"%s",strrchr(p,'/')?strrchr(p,'/')+1:p);snprintf(t->path,sizeof(t->path),"%s",p);snprintf(t->tunnel,sizeof(t->tunnel),"%s",a->chat_tunnel);t->size=fsize;t->started=now_millis();memcpy(t->digest,dig,32U);int rc=net_send_file_request(a->net,a->chat_tunnel,t->id,t->filename,t->size,t->digest);ui_status(a,rc==BALANCE_OK?"file [--------------------] 0.0% · waiting for acceptance":"file request failed");return rc;}
    if(strcmp(name,"cloud")==0){if(argc==1){char m[768];snprintf(m,sizeof(m),"cloud=%s · url=%s · token=%s",a->vault->cloud_enabled?"on":"off",a->vault->cloud_url[0]?a->vault->cloud_url:"not configured",a->vault->cloud_token[0]?"configured":"not set");ui_add_msg(a,m,false);return BALANCE_OK;}if(argc>=2&&strcmp(argv[1],"off")==0){a->vault->cloud_enabled=false;if(store_save_vault_key(a->store,a->vault,a->master_key)!=BALANCE_OK)return BALANCE_ERR;ui_status(a,"cloud disabled · future chats stay volatile");return BALANCE_OK;}if(argc>=3&&strcmp(argv[1],"on")==0){if(!cloud_url_is_allowed(argv[2])){ui_status(a,"cloud URL must be HTTPS (localhost HTTP allowed for development)");return BALANCE_ERR;}a->vault->cloud_enabled=true;snprintf(a->vault->cloud_url,sizeof(a->vault->cloud_url),"%s",argv[2]);if(argc>=4)snprintf(a->vault->cloud_token,sizeof(a->vault->cloud_token),"%s",argv[3]);if(!a->vault->cloud_object_id[0]){uint8_t r[16];if(crypto_random(r,sizeof(r))!=BALANCE_OK)return BALANCE_ERR;crypto_hex(r,sizeof(r),a->vault->cloud_object_id,sizeof(a->vault->cloud_object_id));}if(store_save_vault_key(a->store,a->vault,a->master_key)!=BALANCE_OK)return BALANCE_ERR;ui_status(a,"cloud enabled · only encrypted history leaves this device");return BALANCE_OK;}ui_add_msg(a,"usage: cloud | cloud on <https-url> [token] | cloud off",false);return BALANCE_ERR;}
    if(strcmp(name,"sync")==0){if(!a->vault->cloud_enabled){ui_status(a,"cloud mode is off");return BALANCE_ERR;}int rc=cloud_put_history(a->vault,a->store);ui_status(a,rc==BALANCE_OK?"sync [####################] 100% · encrypted history uploaded":"sync failed");return rc;}
    if(strcmp(name,"pull")==0){if(!a->vault->cloud_enabled){ui_status(a,"cloud mode is off");return BALANCE_ERR;}int rc=cloud_get_history(a->vault,a->store);ui_status(a,rc==BALANCE_OK?"pull [####################] 100% · encrypted history restored":"pull failed");return rc;}
    if(strcmp(name,"config")==0){char cfg_line[768];snprintf(cfg_line,sizeof(cfg_line),"root %s",a->store->root);ui_add_msg(a,cfg_line,false);snprintf(cfg_line,sizeof(cfg_line),"vault %s",a->store->vault_path);ui_add_msg(a,cfg_line,false);snprintf(cfg_line,sizeof(cfg_line),"history %s",a->store->history_path);ui_add_msg(a,cfg_line,false);snprintf(cfg_line,sizeof(cfg_line),"endpoint %s:%u",a->vault->endpoint[0]?a->vault->endpoint:"0.0.0.0",(unsigned)net_listen_port(a->net));ui_add_msg(a,cfg_line,false);snprintf(cfg_line,sizeof(cfg_line),"cloud %s",a->vault->cloud_enabled?a->vault->cloud_url:"disabled");ui_add_msg(a,cfg_line,false);return BALANCE_OK;}
    if(strcmp(name,"doctor")==0){struct stat st={0};char m[768];int root_ok=stat(a->store->root,&st)==0&&S_ISDIR(st.st_mode)&&((st.st_mode&0777)==0700);int vault_ok=stat(a->store->vault_path,&st)==0&&((st.st_mode&0777)==0600);int hist_ok=access(a->store->history_path,F_OK)!=0|| (stat(a->store->history_path,&st)==0&&((st.st_mode&0777)==0600));int cloud_ok=!a->vault->cloud_enabled||cloud_url_is_allowed(a->vault->cloud_url);snprintf(m,sizeof(m),"doctor · root %s · vault %s · history %s · cloud %s · friends %zu",root_ok?"ok":"check",vault_ok?"ok":"check",hist_ok?"ok":"check",cloud_ok?"ok":"check",a->vault->friend_count);ui_add_msg(a,m,false);return (root_ok&&vault_ok&&hist_ok&&cloud_ok)?BALANCE_OK:BALANCE_ERR;}
    if(strcmp(name,"theme")==0){if(argc<2){ui_add_msg(a,"theme cyber | mono | amber | cycle",false);return BALANCE_OK;}if(strcmp(argv[1],"cycle")==0)a->theme=(ThemeMode)((a->theme+1)%3);else if(strcmp(argv[1],"cyber")==0)a->theme=THEME_CYBER;else if(strcmp(argv[1],"mono")==0)a->theme=THEME_MONO;else if(strcmp(argv[1],"amber")==0)a->theme=THEME_AMBER;else{ui_status(a,"unknown theme");return BALANCE_ERR;}theme_apply(a);ui_status(a,"theme updated");return BALANCE_OK;}
    if(strcmp(name,"redraw")==0){layout(a);return BALANCE_OK;}
    if(strcmp(name,"clear")==0){clear_view(a);return BALANCE_OK;}
    if(strcmp(name,"quit")==0||strcmp(name,"exit")==0){a->exit_requested=true;return BALANCE_OK;}
    return BALANCE_ERR;
}

static void apply_event(App*a,const NetEvent*ev){if(!a||!ev)return;switch(ev->type){
    case NET_EV_STATUS:ui_status(a,ev->text);break;
    case NET_EV_TUNNEL_REQUEST:{char c[128];snprintf(c,sizeof(c),"accept %s",ev->tunnel_id);action_add(a,"accept tunnel",c);snprintf(c,sizeof(c),"reject %s",ev->tunnel_id);action_add(a,"reject tunnel",c);snprintf(a->chat_peer,sizeof(a->chat_peer),"%s",ev->username);ui_add_msg(a,"incoming tunnel request · choose accept/reject",false);break;}
    case NET_EV_TUNNEL_ACCEPTED:snprintf(a->chat_peer,sizeof(a->chat_peer),"%s",ev->username);snprintf(a->chat_tunnel,sizeof(a->chat_tunnel),"%s",ev->tunnel_id);a->view=VIEW_CHAT;load_chat_history(a,a->chat_peer);ui_add_msg(a,"tunnel connected · end-to-end encrypted",false);break;
    case NET_EV_TUNNEL_REJECTED:clear_volatile_chat(a);a->view=VIEW_HOME;ui_add_msg(a,ev->reason[0]?ev->reason:"tunnel rejected",false);break;
    case NET_EV_MESSAGE:if(strcmp(a->chat_tunnel,ev->tunnel_id)==0){ui_add_msg(a,ev->text,false);if(a->vault->cloud_enabled){HistoryEntry he={.ts=now_millis(),.local=false};snprintf(he.chat,sizeof(he.chat),"%s <-> %s",a->current_user,a->chat_peer);snprintf(he.text,sizeof(he.text),"%s",ev->text);(void)store_append_history(a->store,a->master_key,&he);(void)cloud_put_history(a->vault,a->store);}}break;
    case NET_EV_FILE_REQUEST:{Transfer*t=transfer_new(a,ev->transfer_id);if(t){t->receiving=true;t->size=ev->size;t->started=now_millis();t->accepted=false;memcpy(t->digest,ev->digest,32U);snprintf(t->filename,sizeof(t->filename),"%s",ev->filename);snprintf(t->tunnel,sizeof(t->tunnel),"%s",ev->tunnel_id);char s1[128],s2[256],dir[BALANCE_MAX_PATH];sanitize_component(a->current_user,s1,sizeof(s1));sanitize_component(ev->username,s2,sizeof(s2));snprintf(dir,sizeof(dir),"transfers/%s--%s",s1,s2);mkdir("transfers",0700);mkdir(dir,0700);snprintf(t->path,sizeof(t->path),"%s/%s.part",dir,t->id);char ac[128],rc[128];snprintf(ac,sizeof(ac),"accept %s",t->id);snprintf(rc,sizeof(rc),"reject %s",t->id);action_add(a,"accept file",ac);action_add(a,"reject file",rc);ui_status(a,"file [--------------------] 0.0% · waiting for acceptance");ui_add_msg(a,"incoming file request · choose accept/reject",false);}}break;
    case NET_EV_FILE_ACCEPTED:{Transfer*t=transfer_find(a,ev->transfer_id);if(t){t->accepted=true;t->sender=true;if(!t->sender_running){SendArgs*sa=calloc(1,sizeof(*sa));if(sa){sa->a=a;snprintf(sa->id,sizeof(sa->id),"%s",t->id);snprintf(sa->path,sizeof(sa->path),"%s",t->path);snprintf(sa->tunnel,sizeof(sa->tunnel),"%s",t->tunnel);sa->size=t->size;if(pthread_create(&t->sender_thread,NULL,send_file_worker,sa)==0)t->sender_running=true;else free(sa);}}ui_status(a,"file accepted · starting encrypted transfer");}}break;
    case NET_EV_FILE_REJECTED:{Transfer*t=transfer_find(a,ev->transfer_id);if(t){ui_add_msg(a,ev->reason[0]?ev->reason:"file rejected",false);memset(t,0,sizeof(*t));t->fd=-1;}}break;
    case NET_EV_FILE_CHUNK:{Transfer*t=transfer_find(a,ev->transfer_id);if(t&&t->receiving&&t->accepted){if(ev->offset!=t->done){ui_status(a,"file chunk order error · transfer discarded");unlink(t->path);t->finished=true;break;}if(t->fd<0)t->fd=open(t->path,O_WRONLY|O_CREAT|O_APPEND,0600);if(t->fd>=0){ssize_t wr=write(t->fd,ev->data,ev->data_len);if(wr==(ssize_t)ev->data_len)t->done+=ev->data_len;uint64_t now=now_millis();char bar[128],spd[64],st[256];format_progress(t->done,t->size,bar,sizeof(bar));uint64_t elapsed=now>t->started?now-t->started:1ULL;format_speed((t->done*1000ULL)/elapsed,spd,sizeof(spd));snprintf(st,sizeof(st),"recv %s %s · %s",t->filename,bar,spd);ui_status(a,st);}}}break;
    case NET_EV_FILE_DONE:{Transfer*t=transfer_find(a,ev->transfer_id);if(t&&t->receiving){if(t->fd<0)t->fd=open(t->path,O_WRONLY|O_CREAT|O_APPEND,0600);if(t->fd>=0){fsync(t->fd);close(t->fd);t->fd=-1;}int fd=open(t->path,O_RDONLY);if(fd>=0){uint8_t buf[65536],dig[32];ssize_t nr;uint64_t total=0;EVP_MD_CTX*md=EVP_MD_CTX_new();bool good=md&&EVP_DigestInit_ex(md,EVP_sha256(),NULL)==1;while(good&&(nr=read(fd,buf,sizeof(buf)))>0){if(EVP_DigestUpdate(md,buf,(size_t)nr)!=1)good=false;total+=(uint64_t)nr;}unsigned int dl=32;if(good)good=EVP_DigestFinal_ex(md,dig,&dl)==1;if(md)EVP_MD_CTX_free(md);close(fd);good=good&&total==t->size&&CRYPTO_memcmp(dig,t->digest,32U)==0;if(good){char final_path[BALANCE_MAX_PATH],*slash=strrchr(t->path,'/');if(slash){size_t dl2=(size_t)(slash-t->path);char dir[BALANCE_MAX_PATH];if(dl2<sizeof(dir)){memcpy(dir,t->path,dl2);dir[dl2]='\0';if(final_transfer_path(dir,t->filename,t->path,final_path,sizeof(final_path))==BALANCE_OK){ui_add_msg(a,"file received · verified SHA-256 · finalized",false);}}}}else{ui_add_msg(a,"file verification failed · discarded",false);unlink(t->path);}}}if(t){char c[128];snprintf(c,sizeof(c),"accept %s",t->id);action_remove(a,c);snprintf(c,sizeof(c),"reject %s",t->id);action_remove(a,c);t->finished=true;}break;}
    case NET_EV_DISCONNECTED:ui_status(a,"peer disconnected · volatile tunnel closed");if(a->view==VIEW_CHAT){clear_volatile_chat(a);a->view=VIEW_HOME;}break;
    case NET_EV_FRIEND_REQUEST:{Friend*f=NULL;for(size_t i=0;i<a->vault->friend_count;++i)if(strcmp(a->vault->friends[i].username,ev->username)==0)f=&a->vault->friends[i];if(!f&&a->vault->friend_count<MAX_FRIENDS){f=&a->vault->friends[a->vault->friend_count++];memset(f,0,sizeof(*f));snprintf(f->username,sizeof(f->username),"%s",ev->username);}if(f){snprintf(f->endpoint,sizeof(f->endpoint),"%s",ev->endpoint);memcpy(f->ed_pub,ev->ed_pub,32U);f->key_pinned=false;}char c[256];snprintf(c,sizeof(c),"accept %s",ev->username);action_add(a,"accept friend",c);snprintf(c,sizeof(c),"reject %s",ev->username);action_add(a,"reject friend",c);ui_add_msg(a,"incoming friend request",false);break;}
    case NET_EV_FRIEND_ACCEPTED:{Friend*f=NULL;for(size_t i=0;i<a->vault->friend_count;++i)if(strcmp(a->vault->friends[i].username,ev->username)==0)f=&a->vault->friends[i];if(!f&&a->vault->friend_count<MAX_FRIENDS){f=&a->vault->friends[a->vault->friend_count++];memset(f,0,sizeof(*f));snprintf(f->username,sizeof(f->username),"%s",ev->username);}if(f){memcpy(f->ed_pub,ev->ed_pub,32U);f->key_pinned=true;snprintf(f->endpoint,sizeof(f->endpoint),"%s",ev->endpoint);store_save_vault_key(a->store,a->vault,a->master_key);}char c[128];snprintf(c,sizeof(c),"accept %s",ev->username);action_remove(a,c);snprintf(c,sizeof(c),"reject %s",ev->username);action_remove(a,c);ui_add_msg(a,"friend request accepted · key pinned",false);break;}
    case NET_EV_FRIEND_REJECTED:ui_add_msg(a,"friend request rejected",false);break;
    default:break;}}

void ui_on_net_event(const NetEvent*ev,void*opaque){App*a=opaque;if(!a||!ev)return;QNode*n=calloc(1,sizeof(*n));if(!n)return;n->ev=*ev;if(ev->data&&ev->data_len){n->data=malloc(ev->data_len);if(!n->data){free(n);return;}memcpy(n->data,ev->data,ev->data_len);n->ev.data=n->data;}pthread_mutex_lock(&a->q_mu);if(a->q_tail)a->q_tail->next=n;else a->q_head=n;a->q_tail=n;pthread_mutex_unlock(&a->q_mu);}
static void drain_events(App*a){for(;;){pthread_mutex_lock(&a->q_mu);QNode*n=a->q_head;if(n){a->q_head=n->next;if(!a->q_head)a->q_tail=NULL;}pthread_mutex_unlock(&a->q_mu);if(!n)break;apply_event(a,&n->ev);free(n->data);free(n);}}

static void submit(App*a){
    char line[UI_INPUT_MAX];snprintf(line,sizeof(line),"%s",a->input);fill_command(a,"");trim_inplace(line);if(!line[0])return;
    if(line[0]=='?'){remember_command(a,line);command_exec(a,line);return;}
    if(a->view==VIEW_CHAT){
        if(net_send_chat(a->net,a->chat_tunnel,line)==BALANCE_OK){ui_add_msg(a,line,true);
            if(a->vault->cloud_enabled){HistoryEntry he={.ts=now_millis(),.local=true};snprintf(he.chat,sizeof(he.chat),"%s <-> %s",a->current_user,a->chat_peer);snprintf(he.text,sizeof(he.text),"%s",line);(void)store_append_history(a->store,a->master_key,&he);(void)cloud_put_history(a->vault,a->store);}}
        else ui_status(a,"send failed");
    }else ui_status(a,"home is command-driven · use ? for commands");
}

static int auth_line(WINDOW*w,int y,int x,int width,char*out,size_t cap,bool secret){size_t len=0,pos=0;out[0]='\0';curs_set(1);for(;;){wmove(w,y,x);for(int i=0;i<width;++i)waddch(w,' ');if(secret){wmove(w,y,x);for(size_t i=0;i<len&&i+1U<(size_t)width;++i)waddch(w,'*');}else if(len)mvwaddnstr(w,y,x,out,(int)width-1);wmove(w,y,x+((int)pos<width-1?(int)pos:width-2));wrefresh(w);int ch=wgetch(w);if(ch==27)return BALANCE_ERR;if(ch=='\n'||ch==KEY_ENTER){if(!len)return BALANCE_ERR;return BALANCE_OK;}if(ch==KEY_BACKSPACE||ch==127){if(pos){memmove(out+pos-1U,out+pos,len-pos);--len;--pos;out[len]='\0';}continue;}if(ch==KEY_LEFT){if(pos)--pos;continue;}if(ch==KEY_RIGHT){if(pos<len)++pos;continue;}if(ch==KEY_HOME){pos=0;continue;}if(ch==KEY_END){pos=len;continue;}if(isprint(ch)&&len+1U<cap){memmove(out+pos+1U,out+pos,len-pos);out[pos]=(char)ch;++len;++pos;out[len]='\0';}}
}

int ui_authenticate(Store*store,Vault*vault,uint8_t master_key[32]){
    if(!store||!vault||!master_key)return BALANCE_ERR;
    initscr(); raw(); noecho(); keypad(stdscr,TRUE); curs_set(1); timeout(-1);
    bool registering=!store_exists(store);
    char username[BALANCE_MAX_USERNAME]={0},password[256]={0},repeat[256]={0},error[160]={0};
    int rc=BALANCE_ERR;
    for(;;){
        werase(stdscr);
        int rows,cols; screen_size(&rows,&cols);
        int boxw=cols>76?76:cols-4; if(boxw<42)boxw=42;
        int boxh=registering?15:11; if(boxh>rows-2)boxh=rows-2;
        int x=(cols-boxw)/2, y=(rows-boxh)/2; if(y<1)y=1;
        for(int xx=x;xx<x+boxw;xx++){mvwaddch(stdscr,y,xx,'-');mvwaddch(stdscr,y+boxh-1,xx,'-');}
        for(int yy=y+1;yy<y+boxh-1;yy++){mvwaddch(stdscr,yy,x,'|');mvwaddch(stdscr,yy,x+boxw-1,'|');}
        center_line(stdscr,y+1,cols,registering?"BALANCE  /  CREATE ACCOUNT":"BALANCE  /  UNLOCK",A_BOLD);
        mvwaddnstr(stdscr,y+3,x+3,registering?"Create a local encrypted vault":"Unlock the local encrypted vault",boxw-6);
        int fy=y+5;
        if(registering){
            mvwaddnstr(stdscr,fy,x+3,"username",boxw-6);
            if(auth_line(stdscr,fy+1,x+3,boxw-7,username,sizeof(username),false)!=BALANCE_OK)break;
            fy+=3;
        }
        mvwaddnstr(stdscr,fy,x+3,"password",boxw-6);
        if(auth_line(stdscr,fy+1,x+3,boxw-7,password,sizeof(password),true)!=BALANCE_OK)break;
        if(registering){
            mvwaddnstr(stdscr,fy+3,x+3,"repeat password",boxw-6);
            if(auth_line(stdscr,fy+4,x+3,boxw-7,repeat,sizeof(repeat),true)!=BALANCE_OK)break;
            if(strlen(username)==0||strlen(username)>=BALANCE_MAX_USERNAME||strlen(password)<10U||strcmp(password,repeat)!=0){
                snprintf(error,sizeof(error),"username/password details are invalid"); secure_zero(password,sizeof(password));secure_zero(repeat,sizeof(repeat));
                mvwaddnstr(stdscr,y+boxh-3,x+3,error,boxw-6); mvwaddnstr(stdscr,y+boxh-2,x+3,"Press Enter to try again",boxw-6); wrefresh(stdscr); while(wgetch(stdscr)!='\n'){} continue;
            }
            if(store_register(store,username,password,BALANCE_DEFAULT_LISTEN,vault)!=BALANCE_OK){
                snprintf(error,sizeof(error),"registration failed; the vault was not created"); secure_zero(password,sizeof(password));secure_zero(repeat,sizeof(repeat));
                mvwaddnstr(stdscr,y+boxh-3,x+3,error,boxw-6); mvwaddnstr(stdscr,y+boxh-2,x+3,"Press Enter to try again",boxw-6); wrefresh(stdscr); while(wgetch(stdscr)!='\n'){} continue;
            }
        }else if(store_login(store,password,vault)!=BALANCE_OK){
            snprintf(error,sizeof(error),"unlock failed; check the password and try again");
            secure_zero(password,sizeof(password));
            mvwaddnstr(stdscr,y+boxh-3,x+3,error,boxw-6); mvwaddnstr(stdscr,y+boxh-2,x+3,"Press Enter to try again",boxw-6); wrefresh(stdscr); while(wgetch(stdscr)!='\n'){} continue;
        }
        if(store_master_key(store,password,master_key,(uint8_t[16]){0})!=BALANCE_OK){vault_free(vault);break;}
        rc=BALANCE_OK; secure_zero(password,sizeof(password));secure_zero(repeat,sizeof(repeat));break;
    }
    secure_zero(password,sizeof(password));secure_zero(repeat,sizeof(repeat));endwin();return rc;
}
int ui_run(Store*store,Vault*vault,NetContext*net,uint8_t master_key[32]){
    App *a=calloc(1,sizeof(*a));
    if(!a)return BALANCE_ERR;
    a->store=store;a->vault=vault;a->net=net;a->running=true;a->cmd_history_pos=0;
    snprintf(a->current_user,sizeof(a->current_user),"%s",vault->username);memcpy(a->master_key,master_key,32U);
    pthread_mutex_init(&a->mu,NULL);pthread_mutex_init(&a->q_mu,NULL);
    WINDOW*scr=initscr();(void)scr;raw();noecho();keypad(scr,TRUE);theme_init(a);mousemask(ALL_MOUSE_EVENTS,NULL);curs_set(1);timeout(80);layout(a);
    while(a->running&&!a->exit_requested){
        drain_events(a);
        int ch=wgetch(a->w_input);
        if(ch==KEY_RESIZE){layout(a);continue;}
        if(ch==ERR){redraw_chat(a);redraw_header(a);if(a->w_side)redraw_side(a);redraw_actions(a);redraw_input(a);continue;}
        if(ch==12){layout(a);continue;}
        if(ch==4&&a->input_len==0){a->exit_requested=true;continue;}
        if(ch==27){if(a->view==VIEW_CHAT){a->view=VIEW_HOME;clear_volatile_chat(a);redraw_header(a);redraw_chat(a);if(a->w_side)redraw_side(a);redraw_actions(a);redraw_input(a);}continue;}
        if(ch==9){complete_prefix(a);redraw_input(a);continue;}
        if(ch==KEY_BACKSPACE||ch==127){input_back(a);redraw_input(a);continue;}
        if(ch==KEY_DC){input_del(a);redraw_input(a);continue;}
        if(ch==KEY_LEFT){if(a->input_pos)a->input_pos--;redraw_input(a);continue;}
        if(ch==KEY_RIGHT){if(a->input_pos<a->input_len)a->input_pos++;redraw_input(a);continue;}
        if(ch==KEY_HOME){a->input_pos=0;redraw_input(a);continue;}
        if(ch==KEY_END){a->input_pos=a->input_len;redraw_input(a);continue;}
        if(ch==KEY_UP){if(a->view==VIEW_CHAT){a->scroll++;redraw_chat(a);}else{history_nav(a,-1);redraw_input(a);}continue;}
        if(ch==KEY_DOWN){if(a->view==VIEW_CHAT){if(a->scroll)a->scroll--;redraw_chat(a);}else{history_nav(a,1);redraw_input(a);}continue;}
        if(ch==KEY_PPAGE){a->scroll+=5U;if(a->view==VIEW_CHAT)redraw_chat(a);continue;}
        if(ch==KEY_NPAGE){a->scroll=a->scroll>5U?a->scroll-5U:0U;if(a->view==VIEW_CHAT)redraw_chat(a);continue;}
        if(ch=='\n'||ch==KEY_ENTER){submit(a);redraw_header(a);redraw_chat(a);if(a->w_side)redraw_side(a);redraw_actions(a);redraw_input(a);continue;}
        if(ch==KEY_MOUSE){MEVENT me;if(getmouse(&me)==OK){
            if(me.bstate&BUTTON1_CLICKED && a->side_w>0 && me.x>=a->cols-a->side_w && me.y>=1 && me.y<1+a->chat_h){
                int ly=me.y-1, lx=me.x-(a->cols-a->side_w);
                if(a->view==VIEW_HOME){int row=3;for(size_t i=0;i<a->vault->friend_count;i++){if(ly==row){char cmd[256];snprintf(cmd,sizeof(cmd),"?chat %s",a->vault->friends[i].username);fill_command(a,cmd);break;}row+=2;}}
                else for(size_t i=0;i<a->action_count;i++)if(ly>=a->actions[i].y0&&ly<=a->actions[i].y1){char cmd[256];snprintf(cmd,sizeof(cmd),"?%s",a->actions[i].command);fill_command(a,cmd);break;}
                (void)lx;redraw_input(a);continue;
            }
        }}
        if(isprint(ch)){input_insert(a,ch);redraw_input(a);}
    }
    drain_events(a);for(size_t i=0;i<UI_MAX_TRANSFERS;++i)if(a->transfers[i].sender_running)pthread_join(a->transfers[i].sender_thread,NULL);
    if(a->w_side)delwin(a->w_side);
    endwin();pthread_mutex_destroy(&a->q_mu);pthread_mutex_destroy(&a->mu);secure_zero(a->master_key,sizeof(a->master_key));free(a);return BALANCE_OK;
}
