
// Usage of exceptions
function msexception(msg, errid) {
    this.error = errid;
    this.message = msg;
}

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

// Usage: var x = new msuser();
function msuser(token, xhobject, f_operate, u_operate) {

    this.xhobject = xhobject;
    this.token = token;
    this.f_operate = f_operate;
    this.u_operate = u_operate;

    // Operations
    this.openfile = function (filename, operate) {
        this.xhobject.open("GET", this.f_operate + "?operate=open&name=" + filename + "&utoken=" + this.token + "&type=" + operate, false);
        this.xhobject.send(null);
        if (this.xhobject.status != 200) {
            throw new msexception("Permission denied", 1);
        } else {
            return new msfile(this.xhobject.responseText, this.xhobject, this.f_operate);
        }
    }

    this.chown = function (filename, chto) {
        this.xhobject.open("GET", this.u_operate + "?operate=chown&file=" + filename + "&token=" + this.token + "&touid=" + chto, false);
        this.xhobject.send(null);
        if (this.xhobject.status != 200) {
            throw new msexception("Permission denied", 1);
        }
    }

    this.chperm = function (filename, chto, perm) {
        // To be implemented...
    }

    this.modify = function (new_pass) {
        // To be implemented...
    }

    this.logout = function () {
        this.xhobject.open("GET", this.u_operate + "?operate=logout&token=" + this.token, false);
        this.xhobject.send(null);
        this.token = null;
    }

}

// Usage: var x = new mslib();

function mslib() {
    this.xhobject = null;

    if (window.XMLHttpRequest) {
        this.xhobject = new XMLHttpRequest();
    } else if (window.ActiveXObject) {
        this.xhobject = new ActiveXObject("Microsoft.XMLHTTP");
    } else {
        throw new msexception("This bowser does not support XMLHTTP for connection!", -1);
    }

    // Operations
    this.f_operate = "/file_operate";
    this.u_operate = "/auth_workspace";
    this.default_user = new msuser(0, this.xhobject);

    this.auth = function (uid, passwd) {
        this.xhobject.open("GET", this.u_operate + "?operate=check&request=" + uid + "&passwd=" + passwd, false);
        this.xhobject.send(null);
        if (this.xhobject.status != 200) {
            throw new msexception("Incorrect user name or password", 2);
        } else {
            return new msuser(this.xhobject.responseText, this.xhobject, this.f_operate, this.u_operate);
        }
    }

    this.register = function (passwd) {
        // To be implemented ...
    }

}