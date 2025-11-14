/*
 * Loader Implementation
 *
 * 2018, Operating Systems
 */
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "exec_parser.h"
#include "utils.h"

#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#define MAX(A, B) (((A) > (B)) ? (A) : (B))

#define SIZE_FACTOR 128

/* Static variables used across functions old_action is the default handler */
static so_exec_t *exec;
static struct sigaction old_action;
static int pagesize;
/* Static variables referring the executable file */
static void *executable_file;
/* Structure used by each segment to manage its pages */
typedef struct _page_vector {
	int size;
	void **pages;
} page_vector;

static void my_handler(int signum, siginfo_t *info, void *context)
{
	int i, j, rc, page_index, old_size;
	int zero_region_start, zero_region_end, zero_region_length;
	void *current_page;
	uintptr_t addr = (uintptr_t)info->si_addr, endOfSegment;
	page_vector *segment_data;

	for (i = 0; i < exec->segments_no; i++) {
		endOfSegment = exec->segments[i].vaddr + exec->segments[i].mem_size;
		if (addr >= exec->segments[i].vaddr && addr < endOfSegment) {
			/* In this segment */
			page_index = (addr - exec->segments[i].vaddr) / pagesize;
			segment_data = (page_vector *)exec->segments[i].data;
			if (segment_data->size < page_index
					|| !segment_data->pages[page_index]) { /* Page not mapped */
				current_page = mmap((void *)exec->segments[i].vaddr +
					page_index * pagesize, pagesize, PROT_WRITE,
						MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
				DIE(current_page == MAP_FAILED, "mmap");
				memcpy(current_page,
					executable_file + exec->segments[i].offset +
						page_index * pagesize,
					pagesize);
				/* Populate memory from file_size to mem_size with 0 */
				if ((page_index + 1) * pagesize > exec->segments[i].file_size) {
					zero_region_start = MAX(exec->segments[i].file_size,
						page_index * pagesize);
					zero_region_end = MIN(exec->segments[i].mem_size,
						(page_index + 1) * pagesize);
					zero_region_length = zero_region_end - zero_region_start;
					if (zero_region_length)
						memset((void *)exec->segments[i].vaddr +
							zero_region_start, 0, zero_region_length);
				}
				/* Update permissions for the page */
				rc = mprotect(current_page, pagesize, exec->segments[i].perm);
				DIE(rc != 0, "mprotect");
				if (segment_data->size < page_index) { /* Realloc page vector */
					old_size = segment_data->size;
					segment_data->size = (page_index / SIZE_FACTOR + 1)
						* SIZE_FACTOR;
					segment_data->pages = realloc(segment_data->pages,
						segment_data->size * sizeof(void *));
					for (j = old_size; j < segment_data->size; j++)
						segment_data->pages[j] = NULL;
				}
				/* Add page to page vector */
				segment_data->pages[page_index] = current_page;
				return;
			}
			/* Already mapped */
			break;
		}
	}
	/* If not in segment run default handler or already mapped */
	old_action.sa_sigaction(signum, info, context);
}

int so_init_loader(void)
{
	struct sigaction sa;

	pagesize = getpagesize();
	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = my_handler;
	sigaction(SIGSEGV, &sa, &old_action);
	return 0;
}

int so_execute(char *path, char *argv[])
{
	int i, j, exec_fd, rc;
	struct stat buf;
	page_vector *segment_data;

	exec = so_parse_exec(path);
	if (!exec)
		return -1;
	/* Initialise the segments data */
	for (i = 0; i < exec->segments_no; i++) {
		exec->segments[i].data = malloc(sizeof(page_vector));
		segment_data = exec->segments[i].data;
		segment_data->size = SIZE_FACTOR;
		segment_data->pages = malloc(SIZE_FACTOR * sizeof(void *));
		for (j = 0; j < SIZE_FACTOR; j++)
			segment_data->pages[j] = NULL;
	}
	/* Initialise the executable in memory */
	exec_fd = open(path, O_RDONLY);
	DIE(exec_fd == -1, "open");
	fstat(exec_fd, &buf);
	executable_file = mmap(NULL, buf.st_size, PROT_READ, MAP_SHARED,
		exec_fd, 0);
	DIE(executable_file == MAP_FAILED, "mmap");
	rc = close(exec_fd);
	DIE(rc, "close");
	so_start_exec(exec, argv);

	return -1;
}
