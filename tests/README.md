# Tests

Below is a description of what each group of tests does.

## TecnicoFS Tests

- Exclusively tests the file system operations, such as: tfs_open, tfs_close, tfs_link, tfs_symlink, tfs_unlink, tfs_read, etc. It also tries to test that the filesystem is indeed thread safe on all of its operations.

## Producer Consumer Queue Tests

- Simple tests for the producer consumer queue used internally by the mbroker server. Since this is an important piece of the puzzle, it has its own set of tests that make sure no deadlocks or data races occur on the basic operations of the queue.

## Server Tests

- Just a .txt of test ideas for the mbroker server. Ideally we would have shell scripts for this, but we didn't have time to implement them, therefore we decided to just save the basic ideas for ease of access.
