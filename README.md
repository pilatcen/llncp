# Low Latency Network Copy
This is pure C TCP/IP network protocol compatible with Linux or Solaris operating system focused on low latency between independent file transfers. This protocol is lightweight and simple - this means fast. LLNCP can send files only one-by-one (recursive file copy can be added in the future), if you need copy bulk of files over the network, this protocol isn't for you. Use for example RSYNC or SSHFS instead.

### Features
*  Holds permanent network connection between server and client machines
*  Automatic network connection recovery when the network connection between server and client is broken
*  Low latency and fast transport of one-by-one sended files (no repetitious TCP handshake needed when you want to send file - this is achieved by feature number 1.)
*  This protocol can be easy tunelled over SSH
*  Multi threaded server

### Disadvantages
*  No encryption. If you want encryption, use SSH (or similar) tunneling
*  Shared password for all clients connected to one server instance.
*  Can copy only one file per command. No recursive directory copy (easy to add this feature, just create me an Github issue in this repo)

## Compilation Dependencies on Solaris OS
*  install packages *gcc45* (or another version of gcc) and package *developer-gnu*

## Compilation Dependencies on GNU/Linux OS
*  install packages *build-essential*

## Compilation
```
make
```


## Usage

This utility consist from 3 binaries: llncp-server, lncp-clientd, llncp

**llncp-server** runs on server machine, accepts *llncp-clientd* connection

	./llncp-server port logfile password [chroot_dir]	
		'port' is port number (strongly recommended higher than 1024). The server will listen on this port.
		'logfile' is a file used for log output. When 'logfile' is '-', logfile will be the stderr.
		'password' is used for server-client authentication.
		'chroot_dir' is optional. All incomming files can be chrooted in this chroot directory.

**llncp-clientd** is daemon running on the client machine. Is connected to llncp-server and *llncp* conects to this daemon

	./llncp-clientd server_address server_port logfile [password]	
		'server_address' is the IP address of the server where you want send your files.
		'server_port' is an open port on your server with listening llncp-server.
		'logfile' is a file used for log output.
		'password' is used for server-client authentication. Password can be written as the 3rd argument; If not provided, the program will ask the password after its execution.

**llncp** is command for copy files over from client to server

	./llncp server_address source destination.
		'server_address' is server IP/hostname with running LLCP server.
		'source' is absolute path on your local machine to file you want to send.
		'destination' is absolute path to the remote location on remote server.




### Without SSH tunel

1. Server side

		# On the server machine run *llncp-server*. We will start the server instance on IP 10.0.0.1, listening on port 5000,  files can be saved where client wants if server instance has necessary permission, logs will be saved to llncp-server.log file and password is "some_pass".
		
		./llncp-server 5000 llncp-server.log some_pass
2. Client side

		# On the client system run the *llncp-clientd*. Log output will be llncp-clientd.log file.
		
		./llncp-clientd 10.0.0.1 5000 llncp-clientd.log some_pass
		
		# Copy some file from client machine to the server machine
		./llncp 10.0.0.1 /path/to/local/file/on/client/machine /path/on/server/where/file/will/be/stored


### With SSH tunel
Protocol LLNCP doesn't have an encryption layer. If you want encryption, you have to use some form of tunneling, for example SSH. First install the package autossh for autoconnect on ssh tunnel failure.

1. Server side
		
		./llncp-server 5000 llncp-server.log some_pass
		
2. Client side
		
		# First create SSH tunnel to server machine:
		nohup autossh -fN -L 5000:localhost:5000 user@server
		
		./llncp-clientd localhost 5000 llncp-clientd.log some_pass
		
		# SSH tunnel listen on client machine localhost:5000
		./llncp-clientd localhost 5000 llncp-clientd.log some_pass
		
		# Copy some file from client machine to the server machine, server addres is the same
		./llncp 10.0.0.1 /path/to/local/file/on/client/machine /path/on/server/where/file/will/be/stored
	
	

 


