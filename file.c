#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>


int main (int argc, char *argv[]) {
  char buf = malloc(1024*sizeof(char));
  const char *file = "./outfile";

  if (!fread(buf, sizeof(buf), STDIN)) {
    perror(fread);
    return 1;
  }

  int fl = creat(*file, 755);

  if (fl == -1) {
    perror(creat);
    return 1;
  }

  if (!fwrite(&buf, sizeof(buf),1,fl)) {
    perror(fwrite);
    return(1);
  }
  fclose(fl);
  return 0;
}
