
#include "irq.hpp"
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include "rf24_util.hpp"

#define GPIO_PATH "/sys/class/gpio/"
#define MAX_BUF   256+1



const char *RF24Irq::DIR_INPUT="in";
const char *RF24Irq::DIR_OUTPUT="out";
const char *RF24Irq::EDGE_BOTH="both";
const char *RF24Irq::EDGE_RISING="rising";
const char *RF24Irq::EDGE_FALLING="falling";
const char *RF24Irq::EDGE_NONE="none";

RF24Irq::RF24Irq(uint8_t _pin) : pin(_pin), fd(-1) , val(0xFF) {

}

RF24Irq::~RF24Irq() {
  // close the fd if needed
  if(fd!=-1) close(fd);
  // Unexport needed
  unexport();
}

void RF24Irq::un_export(int p) {
  FILE *f;
	f = fopen( GPIO_PATH "unexport", "w");
  if(f!=NULL) {
     fprintf(f, "%d\n", p);
	   fclose(f);
  }
}

void RF24Irq::unexport(){
    RF24Irq::un_export(pin);
}

inline void RF24Irq::clear() {
  lseek(fd,0,SEEK_SET);
  read(fd,&val,1);
  /*int pending,i,pb,pr;
  lseek(fd,0,SEEK_SET);
  ioctl(fd,FIONREAD,&pending);
  pb=pending / sizeof(clear_buffer);
  pr=pending % sizeof(clear_buffer);
  //printf("buffer management -> pending:%i,blocks:%i,remainder:%i\n",pending,pb,pr);
  for(i=0;i<pb;i++) read(fd,clear_buffer,sizeof(clear_buffer)); // Read blocks
  if(pr>0) read(fd,clear_buffer,pr); // read rest of bytes pending */
}

bool RF24Irq::configure(const char* mode,const char *edge) {
  if(fd!=-1) return false;

  FILE *f;
  f = fopen( GPIO_PATH "export", "w");
  if(f==NULL) return false;
  fprintf(f, "%d\n", pin);
  fclose(f);

  char file[MAX_BUF];
  sprintf(file, GPIO_PATH "gpio%d/direction", pin);
  // Export could take a while to be accesible in the fs. wait 10s
  //sleep_us(100000);
  int count=0;
  while(count <10 && access(file,F_OK | W_OK) != 0) {
    //sleep_us(500000); // 100ms
    sleep(1);
    count++;
  }

  if(access(file,F_OK | W_OK) != 0) {
    printf("%s", file);
    unexport();
    return false;
  }


  f=fopen(file,"w");
  if(f==NULL) return false;
  if(fprintf(f, "%s\n",mode)!= (int)((strlen(mode) +1))){
      unexport();
      fclose(f);
      return false;
  }
  fclose(f);

  sprintf(file, GPIO_PATH "gpio%d/edge", pin);
  if(access(file,F_OK | W_OK) != 0) {
    unexport();
    return false;
  }
  f=fopen(file,"w");
  if(f==NULL) return false;
  if(fprintf(f,"%s\n",edge) != (int)((strlen(edge) +1))) {
    unexport();
    fclose(f);
    return false;
  }
  fclose(f);
  return true;

}

bool RF24Irq::begin(const char *mode,const char *edge){

  int count;
  char file[MAX_BUF];

  if(fd>0){
    close(fd);
    unexport();
    fd=-1;
  }
  if(!configure(mode,edge)){
    unexport();
    return false;
  }

  sprintf(file, GPIO_PATH "gpio%d/value", pin);
  if(access(file, F_OK | R_OK) !=0) {
    unexport();
    return false;
  }

  fd=open(file, O_RDONLY /* | O_NONBLOCK*/ );
  if(fd==-1) {
    unexport();
    return false;
  }
  memset(&pfd,0,sizeof(pfd));
  pfd.fd=fd;
  pfd.events= POLLPRI | POLLERR;

  // Clean input.
  if(strcmp(mode,"in")==0) {
    ioctl (fd, FIONREAD, &count);
    for (int i = 0 ; i < count ; ++i)
      read (fd, &val, 1) ;
  }

  return true;
}

int  RF24Irq::wait(bool cl,uint32_t timeout){
  if( fd==-1) return -2;

   if(cl) clear();
   //lseek(fd, 0, SEEK_SET);
   //clear();
   int ret=poll(&pfd,1,timeout);
   if( ret >0 ) {
        // IRQ Received.
        clear();
        if(pfd.revents & POLLPRI) return 1;
        if(pfd.revents & POLLERR) ret=-1;
   } else if (ret < 0) ret--; // -2 is call error;
   return ret;

}
