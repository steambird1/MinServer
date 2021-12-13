# MinServer 2
By steambird1

-----
## Why "2"?
The "MinServer 1", or the older version of MinServer, is [here](https://github.com/steambird1/MinServer-Old).

## Background
On Nov 2021, I decided to make a server application that easy to build and run, and it'll support file operation and user authorize/register in JavaScript to run directly on HTML page.
So that's it :)

## Install
You just need to extract files in latest release and just run `MinServer 2.exe` or compile it by yourself using Visual Studio 2017 or newer.
(Note: There may be some test projects like 'Tester DLL'. You don't need to use it but you can test your server by them.)

There are some help in `help.txt`. You can also read them by running `MinServer 2.exe --help`.

## Use
### Basics Use
You just need to run it. it'll show an information page and it means it's running.
Also, before any setting, you can try visiting `localhost` and you'll got 403 message if it's running normally.

To allow visitors you need to edit `$public.txt` or another file you specified in command line.
Visitors can only visit **paths** you specified in the file (and please notices that `/` is different from `/index.html` although it can mean default file of a directory).

**Notice: There will be a 404 if file allowed to visit but not exist.**

After adding filenames you can try visiting your page again, and you'll see your own page.

### Advanced Use
MinServer is able to do file operation, user authorize in JavaScript and DLL extension in "C++ in `extern C`". To learn more about them, see Wiki (*Maybe implementing*).

## How to help with me (us ?)
Just add Issues and Pull Requests.
Also, please give me a star if you like it :)