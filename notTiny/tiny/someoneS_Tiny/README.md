# csapp_tiny_web_server
Solutions of CSAPP chapter 11 homework 11.6 ~ 11.13

### Command 
- You can use `./tiny <port>` to create listenning server.


#### 11.6 Modify Tiny so that it echoes every request line and request header.
- Add `echo` function which echoes every request line and request header and creates a logfile which store the server output in current working directory.

#### 11.7 Extend Tiny so that it serves MPG video files. Check your work using a real browser.
- Extend `get_fileType` function so that Tiny serves MPG video files.

#### 11.8 Modify Tiny so that it reaps CGI children inside a `SIGCHLD` handler instead of explicitly waiting for them to terminate.
1. Create a sigchld_handler function which reaps CGI children so that Tiny doesn't have to explicitly wait for them to terminate.
2. Add a signal handler error-handling in main function.

#### 11.10 Write an HTML form for the CGI adder function, the form should request content using the GET method.
1. Add html form in `home.html`.
2. Modified original `adder` function to `adder4Web` which handle GET methon URI properly.

#### 11.11 Extend Tiny to support the HTTP HEAD method.
1. Modified `doit` function so it supports not only GET but also HEAD nethod.
2. Add a new function parameter `method` in `serve_static` functio so that function won't send response body to client when request method is HEAD.
3. Add a new function parameter `method` in `serve_dynamic` functio so that function can use it to set CGI variable `REQUEST_METHOD`.
4. Modified original `adder` function to `adder_HEAD` which won't send response body if request method is HEAD.

#### 11.12 Extend Tiny so that it serves dynamic content requested by the HTTP POST method.
Modification
1. Extend Tiny so that it serves dynamic content requested by the HTTP POST method.
2. `read_requesthdrs` function will return the request message body length if request method is POST, else 0.
3. `doit` function use the integer which `read_requesthdrs` function returned to read the request message body in `buf` array, if request method is POST, the `buf` will be the third function parameter of `serve_dynamic` function instead of `cgiargs` array.
