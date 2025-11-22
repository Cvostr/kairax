#include "stdlib.h"
#include "ctype.h"
#include "unistd.h"
#include "string.h"
#include "limits.h"
#include "errno.h"
#include "syscalls.h"
#include "fcntl.h"
#include "sys/stat.h"

char *__realpath(const char *path, char *out)
{
	struct stat fst;
	size_t len;
	ssize_t lnklen;
	char buf[PATH_MAX + 1];
	const char* originalpath = path;

	// Пробуем получить влоб
	ssize_t res = syscall_abspath(AT_FDCWD, path, buf, PATH_MAX + 1);
    if (res > 0)
    {
        memcpy(out, buf, res);
		return out;
    }

	// Пути не существует
	// Возможно, это символьная ссылка, которая указывает на несуществующий файл
	if ((lnklen = readlink(path, buf, PATH_MAX + 1)) > 0)
	{
		memcpy(out, buf, lnklen);
		return out;
	}
	
	// Это не символьная ссылка
	// Тогда выводим так <директория выше>/<имя последнего файла>
	// Переходим на уровень выше
	char *slash = strrchr(path, '/');
	if (slash != NULL)
	{
		// Копируем до /
		memcpy(buf, path, slash - path);
		buf[slash - path] = 0;

		// Информация о файле на уровень выше
		if (stat(buf, &fst) < 0)
		{
			return NULL;
		}

		// Этот файл должен быть директорией
		if (S_ISDIR(fst.st_mode) == 0)
		{
			errno = ENOENT;
			return NULL;
		}

		// Пробуем получить абсолютный путь
		res = syscall_abspath(AT_FDCWD, buf, buf, PATH_MAX + 1);
		if (res < 0)
		{
			// Не смогли получить путь 
			errno = -res;
			return NULL;
		}

		strncpy(out, buf, res);

		// Приделать имя файла на конец
		len = strlen(out);
		if (out[len - 1] != '/')
		{
			out[len] = '/';
			out[len + 1] = '\0';
		}

		strcat(out, slash + 1);
	}
	else
	{
		// Пробуем получить абсолютный путь
		res = syscall_abspath(AT_FDCWD, ".", buf, PATH_MAX + 1);
		if (res < 0)
		{
			// Не смогли получить путь 
			errno = -res;
			return NULL;
		}

		strncpy(out, buf, res);

		// Приделать имя файла на конец
		len = strlen(out);
		if (out[len - 1] != '/')
		{
			out[len] = '/';
			out[len + 1] = '\0';
		}

		strcat(out, path);
	}

	return out;
}

char *realpath(const char *path, char *resolved_path)
{
	char buf[PATH_MAX + 1];

	if (path == NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	if (path[0] == '\0')
	{
		errno = ENOENT;
		return NULL;
	}

	if (resolved_path == NULL)
	{
		resolved_path = malloc(PATH_MAX + 1);
		if (resolved_path == NULL)
		{
			return NULL;
		}
	}

	resolved_path[0] = 0;

	return __realpath(path, resolved_path);
}
