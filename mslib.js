
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
        if (this.xhobject.status != 200) {
            throw new msexception("Bad request", 3);
        } else {
            return this.xhobject.responseText;
        }
    };

    // Write a line
    this.write = function (line) {
        this.xhobject.open("POST", this.f_operate + "?operate=write&token=" + this.token, false);
        this.xhobject.send(line);
        if (this.xhobject.status != 200) {
            throw new msexception("Bad request", 3);
        }
    };

    // Is end-of-file
    this.eof = function () {
        this.xhobject.open("GET", this.f_operate + "?operate=eof&token=" + this.token, false);
        this.xhobject.send(null);
        if (this.xhobject.status != 200) {
            throw new msexception("Bad request", 3);
        } else {
            return this.xhobject.responseText;
        }
    }

    // Close token
    this.close = function () {
        this.xhobject.open("GET", this.f_operate + "?operate=close&token=" + this.token, false);
        this.xhobject.send(null);
        this.token = null;
        this.f_operate = null;
    };

}

// Usage: var x = new msuser();
function msuser(token, xhobject, f_operate, u_operate, d_operate, myuid) {

    this.xhobject = xhobject;
    this.token = token;
    this.f_operate = f_operate;
    this.u_operate = u_operate;
    this.d_operate = d_operate;
    this.myuid = myuid;

    // Operations
    this.openfile = function (filename, operate) {
        if (filename == undefined || operate == undefined) {
            throw new msexception("Bad format of parameter", 6);
        }
        this.xhobject.open("GET", this.f_operate + "?operate=open&name=" + filename + "&utoken=" + this.token + "&type=" + operate, false);
        this.xhobject.send(null);
        if (this.xhobject.status != 200) {
            throw new msexception("Permission denied", 1);
        } else {
            return new msfile(this.xhobject.responseText, this.xhobject, this.f_operate);
        }
    }

    this.chown = function (filename, chto) {
        if (filename == undefined || chto == undefined) {
            throw new msexception("Bad format of parameter", 6);
        }
        this.xhobject.open("GET", this.u_operate + "?operate=chown&file=" + filename + "&token=" + this.token + "&touid=" + chto, false);
        this.xhobject.send(null);
        if (this.xhobject.status != 200) {
            throw new msexception("Permission denied", 1);
        }
    }

    this.chperm = function (filename, chto, perm) {
        if (filename == undefined || chto == undefined || perm == undefined) {
            throw new msexception("Bad format of parameter", 6);
        }
        this.xhobject.open("GET", this.u_operate + "?operate=chperm&file=" + filename + "&token=" + this.token + "&touid=" + chto + "&toperm=" + perm, false);
        this.xhobject.send(null);
        if (this.xhobject.status != 200) {
            throw new msexception("Permission denied", 1);
        }
    }

    this.modify = function (new_pass) {
        if (new_pass == undefined) {
            throw new msexception("Bad format of parameter", 6);
        }
        this.xhobject.send("GET", this.u_operate + "?operate=create&id=" + this.myuid + "&passwd=" + new_pass + "&token=" + this.token, false);
        if (this.xhobject.status != 200) {
            throw new msexception("Permission denied", 1);
        }
    }

    this.upload = function (content, target, jump_to) {
        if (content == undefined || target == undefined || jump_to == undefined) {
            throw new msexception("Bad format of parameter", 6);
        }
        wjumpto = "&jumpto=" + jump_to;
        if (jump_to == undefined) {
            wjumpto = "";
        }
        this.xhobject.open("POST", this.d_operate + "?utoken=" + this.token + "&name=" + target + wjumpto, false);
        this.xhobject.send(content);
        if (this.xhobject.status != 200) {
            throw new msexception("Permission denied", 1);
        }
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
        throw new msexception("This browser does not support XMLHTTP object for connection!", -1);
    }

    // Operations
    this.f_operate = "/file_operate";
    this.u_operate = "/auth_workspace";
    this.d_operate = "/caller";
    this.default_user = new msuser(0, this.xhobject, this.f_operate, this.u_operate, this.d_operate, 0);

    this.auth = function (uid, passwd) {
        if (uid == undefined || passwd == undefined) {
            throw new msexception("Bad format of parameter", 6);
        }
        this.xhobject.open("GET", this.u_operate + "?operate=check&request=" + uid + "&passwd=" + passwd, false);
        this.xhobject.send(null);
        if (this.xhobject.status != 200) {
            throw new msexception("Incorrect user name or password", 2);
        } else {
            return new msuser(this.xhobject.responseText, this.xhobject, this.f_operate, this.u_operate, this.d_operate, uid);
        }
    }

    this.request = function (operator, path, content) {
        if (operator == undefined || path == undefined) {
            throw new msexception("Bad format of parameter", 6);
        }
        this.xhobject.open(operator, path, false);
        this.xhobject.send(content);
        return this.xhobject.responseText;
    }

    this.status = function () {
        return this.xhobject.status;
    }

    this.call = function (script, parameter, method, content) {
        if (script == undefined) {
            throw new msexception("Bad format of parameter", 6);
        }
        wmethod = method;
        if (method == undefined) {
            wmethod = "GET";
        }
        wcontent = content;
        if (content == undefined) {
            wcontent = null;
        }
        wparameter = parameter;
        if (parameter == undefined || parameter == null) {
            wparameter = new Object();
        }
        calls = "";
        for (var i in wparameter) {
            calls += ("&" + i + "=" + wparameter[i]);
        }
        this.xhobject.open(wmethod, this.d_operate + "?module=" + script + calls, false);
        this.xhobject.send(wcontent);
        return this.xhobject.responseText;
    }

    this.register = function (passwd, uid) {
        // To be implemented ...
        if (passwd == undefined) {
            throw new msexception("Bad format of parameter", 6);
        }
        ider = "";
        if (uid == undefined) {
            ider = "";
        } else {
            ider = "&id=" + uid;
        }
        this.xhobject.open("GET", this.u_operate + "?operate=create" + ider + "&passwd=" + passwd, false);
        this.xhobject.send(null);
        if (this.xhobject.status != 200) {
            throw new msexception("User exists and requires login", 2);
        }
        if (ider == "") {
            return this.xhobject.responseText;
        }
    }

    this.file_query = function (token) {
        if (token == undefined) {
            throw new msexception("Bad format of parameter", 6);
        }
        this.xhobject.open("GET", this.f_operate + "?operate=query&token=" + token, false);
        this.xhobject.send(null);
        if (this.xhobject.status != 200) {
            throw new msexception("Specified token not exist", 3);
        } else {
            return this.xhobject.responseText;
        }
    }

    this.user_query = function (token) {
        if (token == undefined) {
            throw new msexception("Bad format of parameter", 6);
        }
        this.xhobject.open("GET", this.u_operate + "?operate=query&token=" + token, false);
        this.xhobject.send(null);
        if (this.xhobject.status != 200) {
            throw new msexception("Specified token not exist", 3);
        } else {
            return this.xhobject.responseText;
        }
    }

    this.list = function () {
        this.xhobject.open("GET", this.f_operate + "?operate=list", false);
        this.xhobject.send(null);
        if (this.xhobject.status != 200) {
            throw new msexception("Unexpected error", 5);
        } else {
            return this.xhobject.responseText;
        }
    }

}

// Some tools useful
// Input: MinServer key-value object (like pageinfo.head_args)
function toJSDictionary(mins_dict_obj) {
    var res = new Object();
    for (var i = 0; i < mins_dict_obj.length; i++) {
        res[mins_dict_obj[i].key] = mins_dict_obj[i].value;
    }
    return res;
}

function translateHTMLChar(hchar) {
    var res = "";
    var i = 0;
    for (; i < hchar.length; i++) {
        if (hchar[i] == "%") {
            res += String.fromCharCode(parseInt(hchar[i + 1] + hchar[i + 2], 16));
            i += 2;
        } else {
            res += hchar[i];
        }
    }
}