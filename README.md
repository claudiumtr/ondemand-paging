# ondemand-paging
"On demand paging" function for memory allocation


My Executable Loader project implement the "On demand paging" function for
memory allocation. The main ideea is starting with no mapped pages and for
each pagefault decide on: mapping a new page or terminate the process based
on siginfo details and executable`s memory zones.
After a page is mapped to its corresponding segment, its populated with
data from the executable and the region from file_size to mem_size is filled
with zeroes, followed by the update of permissions corresponding to the segment
permissions.
I do belive that the homework is a nice way to get accustomed to mmap and
memory zone allocation. I have encountered difficulties at first in the work
with the structures from skelleton, but after understanding their purpose
the homework was quite easy. The implementation is good enough to pass all
tests and has no major drawbacks.
old_action is declared as global static to initialised to 0, therefore
providing the basic implementation for the signal handler.
executable_file is a pointer to the executable mapped. (used for populating new
memory pages).
Make build compiles the sources and so creating a shared library to be used by
other C sources.
