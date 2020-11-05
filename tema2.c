#include <stdlib.h>
#include <string.h>
#include "so_stdio.h"
#include <sys/types.h>  /* open */
#include <sys/stat.h>   /* open */
#include <fcntl.h>  /* O_RDWR, O_CREAT, O_TRUNC, O_WRONLY */
#include <unistd.h> /* close */
#include <stdbool.h>
#include <sys/wait.h>

struct _so_file {
	int fd;
	unsigned char buffer[4096];
	size_t size;
	int index;
	unsigned int w;
	int fseekPos;
	int lastOp;
	bool fEND;
	bool fERR;
	pid_t child_pid;
};
typedef struct _so_file SO_FILE;

SO_FILE *so_fopen(const char *pathname, const char *mode)
{
	SO_FILE *file = calloc(1, sizeof(SO_FILE));

	if (!file)
		return NULL;

	if (strcmp(mode, "r") == 0) {
		file->fd = open(pathname, O_RDONLY, 0777);
	} else if (strcmp(mode, "r+") == 0) {
		file->fd = open(pathname, O_RDWR, 0777);
		file->w = 1;
	} else if (strcmp(mode, "w") == 0) {
		file->fd = open(pathname, O_WRONLY | O_TRUNC | O_CREAT, 0777);
		file->w = 1;
	} else if (strcmp(mode, "w+") == 0) {
		file->fd = open(pathname, O_RDWR | O_TRUNC | O_CREAT, 0777);
		file->w = 1;
	} else if (strcmp(mode, "a") == 0) {
		file->fd = open(pathname, O_APPEND | O_CREAT | O_WRONLY, 0777);
		file->w = 2;
		file->lastOp = -1;
	} else if (strcmp(mode, "a+") == 0) {
		file->fd = open(pathname, O_APPEND | O_CREAT | O_RDWR, 0777);
		file->lastOp = -1;
		file->w = 2;
	} else {
		file->fd = -1;
	}

	if (file->fd < 0) {
		free(file);
		return NULL;
	}
	return file;
}



int so_fileno(SO_FILE *stream)
{
	return stream->fd;
}

int so_fgetc(SO_FILE *stream)
{
	unsigned char elem = 0;

	if (stream->size == 0 || stream->index == stream->size) {
		/*atunci inseamna ca am buffer-ul gol*/
		stream->size = read(stream->fd, stream->buffer, 4096);
		if (stream->size < 0 || stream->size > 4096) {
			stream->fERR = 1;
			return SO_EOF;
		}
		if (stream->size == 0) {
			stream->fEND = 1;
			return SO_EOF;
		}
		stream->index = 0;
	}

	stream->fseekPos++;
	elem = stream->buffer[stream->index];
	stream->index++;
	stream->lastOp = 1;

	return (int)elem;
}

int so_fputc(int c, SO_FILE *stream)
{
	int rc;
	/*setez scrierea la sfarsit de fisier*/
	if (stream->size == 4096) {
		rc = so_fflush(stream);
		if (rc < 0)
			return SO_EOF;
	}
	stream->fseekPos++;
	stream->buffer[stream->size] = (unsigned char) c;
	stream->size++;
	stream->lastOp = 0;
	return c;
}

size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	size_t i = 0;
	int k;

	for (i = 0; i < nmemb * size; i++) {
		k = so_fgetc(stream);
		if (stream->fERR)
			return 0;
		if (k == SO_EOF)
			return (i / size);
		*(unsigned char *)(ptr + i) = (unsigned char)k;
	}
	return (i / size);
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	size_t i = 0;
	unsigned char k;
	int aux;

	for (i = 0; i < nmemb * size; i++) {
		k = *(unsigned char *)(ptr + i);
		aux = so_fputc((int)k, stream);
		if (stream->fERR)
			return 0;
		if (aux == SO_EOF)
			return (i / size);
	}
	return (i / size);
}
int so_fflush(SO_FILE *stream)
{
	size_t nBytes = 0;

	if (stream->lastOp == 1) {
		memset(stream->buffer, 0, 4096);
		stream->size = 0;
		stream->index = 0;
		return 0;
	}
	if (stream->w && stream->size > 0) {
		nBytes = write(stream->fd, stream->buffer, stream->size);
		if (nBytes < 0 || nBytes > 4096) {
			stream->fERR = 1;
			return SO_EOF;
		}

	}
	stream->size = 0;
	stream->index = 0;
	return 0;
}

int so_fclose(SO_FILE *stream)
{
	int rc, cl;

	cl = so_fflush(stream);
	rc = close(stream->fd);
	free(stream);
	if (rc == 0 && cl != -1)
		return rc;
	else
		return SO_EOF;
}

int so_fseek(SO_FILE *stream, long offset, int whence)
{
	int rc;

	so_fflush(stream);
	rc = lseek(stream->fd, offset, whence);
	if (rc < 0)
		return SO_EOF;
	stream->fseekPos = rc;
	return 0;
}

long so_ftell(SO_FILE *stream)
{
	if (stream->fseekPos < 0)
		return SO_EOF;
	return stream->fseekPos;
}

int so_feof(SO_FILE *stream)
{
	return stream->fEND;
}

int so_ferror(SO_FILE *stream)
{
	return stream->fERR;
}

SO_FILE *so_popen(const char *command, const char *type)
{
	pid_t pid;
	int pipeDes[2];
	int rc;
	SO_FILE *stream = NULL;

	/* creez un pipe */
	rc = pipe(pipeDes);
	if (rc == -1)
		return NULL;

	pid = fork();

	switch (pid) {
	case -1: /* Fork error */
		close(pipeDes[0]);
		close(pipeDes[1]);
		return NULL;
	case 0: /* Child process */
/* Close unused pipe heads and redirect stdin/stdout */
/*inchid capetele nefolosite ale pipe-ului si redirectez iesirile*/
		if (type[0] == 'w') {
			dup2(pipeDes[0], STDIN_FILENO);
			close(pipeDes[0]);
			close(pipeDes[1]);

			/*Execute command*/
			execlp("sh", "sh", "-c", command, NULL);
			exit(EXIT_SUCCESS);
		} else if (type[0] == 'r') {
			dup2(pipeDes[1], STDOUT_FILENO);
			close(pipeDes[1]);
			close(pipeDes[0]);
			/*Execute command*/

			execlp("sh", "sh", "-c", command, NULL);
			exit(EXIT_SUCCESS);
		} else {
			close(pipeDes[0]);
			close(pipeDes[1]);
			exit(EXIT_FAILURE);
		}
	default: /* Parent process */
		/* creez structura SO_FILE*/
		stream = calloc(1, sizeof(SO_FILE));
		if (stream == NULL)
			return NULL;
		stream->lastOp = -1;
		stream->child_pid = pid;
/* atribui file descriptorului din structura la capatul necesar al pipe-ului*/
		if (type[0] == 'w') {
			close(pipeDes[0]);
			stream->w = 2;
			stream->fd = pipeDes[1];

		}
		if (type[0] == 'r') {
			close(pipeDes[1]);
			stream->fd = pipeDes[0];

		}
		return stream;
	}
	return stream;
}

int so_pclose(SO_FILE *stream)
{
	int status;
	int pid = stream->child_pid;
	int err = 0;

	err = so_fclose(stream);
	if (err == -1)
		return -1;

	/* astept ca procesul copil sa isi termine executia */
	err = waitpid(pid, &status, 0);
	if (err == -1)
		return -1;
	return status;
}
