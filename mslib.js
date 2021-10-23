
// Usage: var x = new msfile();

function msfile(token, xhobject, f_operate) {
    this.token = token;
    this.xhobject = xhobject;
    this.f_operate = f_operate;

    // Operations

    // Read a line (to '\n')
    this.read = function () {
        this.xhobject.open("GET", this.f_operate + "?operate=read&token=" + this.token, false);
        this.xhobject.send(null);
        return this.xhobject.responseText;
    };

    // Write a line
    this.write = function (line) {
        this.xhobject.open("POST", this.f_operate + "?operate=write&token=" + this.token, false);
        this.xhobject.send(line);
        //return this.xhobject.status;  // Always 200
    };

    // Close token
    this.close = function () {
        this.xhobject.open("GET", this.f_operate + "?operate=close&token=" + this.token, false);
        this.xhobject.send(null);
        this.token = null;
        this.f_operate = null;
    };

}

// Usage: var x = new mslib();

function mslib() {
    this.xhobject = null;
    if (window.XMLHttpRequest) {
        this.xhobject = new XMLHttpRequest();
    } else if (window.ActiveXObject) {
        this.xhobject = new ActiveXObject("Microsoft.XMLHTTP");
    }

    // Operations
    this.f_operate = "/file_operate";

    this.open = function (filename, opt) {
        if (this.xhobject != null) {
            this.xhobject.open("GET", this.f_operate + "?operate=open&name=" + filename + "&type=" + opt, false);
            this.xhobject.send(null);
            //return this.xhobject.responseText;
            return new msfile(this.xhobject.responseText, this.xhobject, this.f_operate);
        }
    };

}