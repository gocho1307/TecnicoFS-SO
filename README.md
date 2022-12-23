# Técnico File System (Project SO)

The base project is TecnicoFS (Técnico File System), a simplified user-mode file system. It is implemented as a library, which can be used by any client process that wants to have a private instance of a file system in which it can keep its data.

Our goal was, in a first moment, to extend the base project with extra features such as, copying from an external file system, allowing the creation and deletion of hard links and soft links to files, and making the file system operations thread-safe.

In the second part of our project, the goal was to build a simple system for publishing and subscribing messages, which are stored in the previously developed file system, TecnicoFS. The system will have a standalone server process, to which different client processes can connect, to publish or receive messages in a given message storage box.

[Project 1 Statement](docs/statement-p1.md) | [Project 2 Statement](docs/statement-p2.md)
