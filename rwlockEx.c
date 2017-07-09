#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>

#define PTHREAD_NUM 10
#define BUFFER_SIZE 128

pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;

void dieWithError(char *msg)
{
	printf("[-]ERROR: %s\n", msg);
	exit(0);
}

void *writer(void *arg)
{
	int fd, string_num = 1;
	char buffer[BUFFER_SIZE];
	time_t rawtime;
	struct tm *timeinfo;

	if ((fd = open("file", O_CREAT|O_WRONLY|O_APPEND, 0744)) < 0)
			dieWithError("open() failed");
	while(1)
	{
		pthread_rwlock_wrlock(&lock);
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		sprintf(buffer, "string[%d]: %d:%d:%d\n", string_num, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
		printf("\nWriter thread[%x]\n", pthread_self());
		if (write(fd, buffer, strlen(buffer)) < 0)
			dieWithError("write() failed");
		string_num++;
		pthread_rwlock_unlock(&lock);
		sleep(1);
	}
	close(fd);
	pthread_exit(0);
}

void *reader(void *arg)
{
	int fd, buffer_read, buffer_written;
	char buffer[BUFFER_SIZE];

	if ((fd = open("file", O_CREAT|O_RDONLY, 0744)) < 0)
			dieWithError("open() failed");
	while(1)
	{
		pthread_rwlock_rdlock(&lock);
		
		printf("\nReader thread[%x]\n", pthread_self());
		while((buffer_read = read(fd, buffer, BUFFER_SIZE)) > 0)
			if ((buffer_written = write(1, buffer, buffer_read)) != buffer_read)
				dieWithError("write() failed");
		pthread_rwlock_unlock(&lock);
		sleep(1);
	}
	close(fd);
	pthread_exit(0);		
}

int main(void)
{
	pthread_t writer_tid;
	pthread_t reader_tid[PTHREAD_NUM];
	int i = 0;

	if (pthread_rwlock_init(&lock, NULL) < 0)
		dieWithError("pthread_rwlock_init() failed");	

	if (pthread_create(&writer_tid, NULL, writer, NULL) < 0)
		dieWithError("Can't create writer thread");
	for (i = 0; i < PTHREAD_NUM; i++)
		if (pthread_create(&reader_tid[i], NULL, reader, NULL) < 0)
			dieWithError("can't create writers thread");

	if (pthread_join(writer_tid, NULL) < 0)
		dieWithError("Join writer thread failed");
	for (i = 0; i < PTHREAD_NUM; i++)
		if (pthread_join(reader_tid[i], NULL) < 0)
			dieWithError("Join reader thread failed");

	if (pthread_rwlock_destroy(&lock) < 0)
		dieWithError("pthread_rwlock_destroy() failed");
	return 0;
}
