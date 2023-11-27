$"use strict";

function I(name) {return document.getElementById(name);}

function A(action, args='') 
{
    if (args) args = '?' + args;
    location.href =  `../action/${action}.html${args}`;
}

function G(pos) 
{
    location.href =  `${pos}.html`;
}

function _door_button(index, second_names, width, disabled_scale, usr_callback)
{
    let result = '<button type="button" class="door-p' + width;
    const is_disabled = (disabled_scale >> index) & 1;
    if (is_disabled) result += ' disabled';
    result += '"'; 
    if (!is_disabled) result += ' onclick="' + usr_callback + '(' + index + ')"';
    result += '>' + (index+1);
    if (second_names)
    {
        result += '<br>';
        let s = second_names[index];
        result += s ? s : "&nbsp";
    }
    return result + '</button>';
}

function _door_tr_line(start_index, second_names, disabled_scale, usr_callback)
{
    const width = start_index != 2 ? [33, 33, 33,] : [25, 25, 50];
    let result = _door_button(start_index, second_names, width[0], disabled_scale, usr_callback);
    if (start_index == 0) result += '<span class="door-p33"></span>';
    else result += _door_button(++start_index, second_names, width[1], disabled_scale, usr_callback);
    return '<tr class="door-p100"><td class="door-p100">' + result + _door_button(++start_index, second_names, width[2], disabled_scale, usr_callback);
}

function door_table(second_names, disabled_scale, usr_callback)
{
    let result = '';
    for(let x of [0, 2, 5]) result += _door_tr_line(x, second_names, disabled_scale, usr_callback);
    return '<div id="downcounter"></div><br><table class="door-table">' + result + '</table>';
}

function loaded_gifts_to_disabled(second_names)
{
    let result = 0;
    let scale = 1;
    if (!second_names) return 0;
    for(let x of second_names)
    {
        if (x) result |= scale;
        scale <<= 1;
    }
    return result;
}

var _time_to_doors_enable = $[TimeToDoorsEnable];

function _msg_time_to_doors()
{
    I("downcounter").innerHTML = _time_to_doors_enable ? "Вы сможете открыть дверцу через " + _time_to_doors_enable + " секунд" : "";
}

function activate_doors(second_names, disabled_scale, usr_callback, reset_timeout = false)
{
    if (reset_timeout) _time_to_doors_enable = 10;
    let doors = I("doors");
    if (_time_to_doors_enable)
    {
        doors.innerHTML = door_table(second_names, 255, "");
        _msg_time_to_doors();
        let cb = () =>
        {
            --_time_to_doors_enable;
            _msg_time_to_doors();
            if (_time_to_doors_enable) setTimeout(cb, 1000); 
            else activate_doors(second_names, disabled_scale, usr_callback);
        };
        setTimeout(cb, 1000);
    }
    else
    {
        doors.innerHTML = door_table(second_names, disabled_scale, usr_callback);
        _msg_time_to_doors();
    }
}

function call_later(cmd, cb)
{
    const timeout  = cmd.timeout ?? 5;
    if (timeout) setTimeout(cb, timeout * 1000); 
}

function set_html_with_timeout(cmd, element_name, msg = undefined)
{
    const element = I(element_name);
    if (!element) return;
    const m = msg ?? cmd.msg ?? '';
    element.innerHTML = m;
    if (m) call_later(cmd, () => {element.innerHTML="";});
}

var websock = null;

// Setup callback handler for WebSocket. Callback got Array of Objects with comands
// Generic commands processed right here, but it still be included in callback argument
function set_async_handler(callback = null)
{
    let port = document.port;
    if (port) port = ':' + port; else port = "";

    if (websock) {alert("set_async_handler called twice!"); return;}
    websock = new WebSocket(`ws://${document.location.host}${port}/notify`);

    websock.onmessage = ({data}) => 
    {
        let command = JSON.parse(data);

        if (!(command instanceof Array)) command = [ command ];

        if (callback) callback(command);

        for(let cmd of command)
        {
            switch(cmd.cmd)
            {
                case "goto":  websock.close(); websock = null; document.location.href = cmd.href; break;
                case "popup": alert(cmd.msg); break;
                case "alert": set_html_with_timeout(cmd, cmd.dst ?? 'alert-target'); break;
                case 'ping': send_ping(cmd); break;
            }
        }
    };
}

window.onload = () => {if (!websock) set_async_handler();};
window.onunload = () => {if (websock) {websock.close(); websock = null;}};


var _popup_data = null;

function show_popup(parent, msg, popup_data)
{
    _popup_data = popup_data;
    const elem = I("popup-text");
    elem.innerHTML = msg;
    const rect = parent.getBoundingClientRect();
    const p_top = rect.top - 10;
    const p_mid = rect.left + (rect.width >> 1);
    elem.classList.add("fgl-show");
    const float_rect = elem.getBoundingClientRect();
    elem.style.top = `${p_top - float_rect.height}px`;
    let left = p_mid - (float_rect.width >> 1);
    if (left < 0) left = 0;
    elem.style.left = `${left}px`;

    window.onclick = ({target}) => {
        if (target != elem && target != parent) 
        {
            elem.classList.remove("fgl-show");
            window.onclick = null;
            document.onkeydown = null;
        }
    };
    document.onkeydown = ({key}) => {
        elem.classList.remove("fgl-show");
        window.onclick = null;
        document.onkeydown = null;
        if (key == 'y' || key == 'Y' || key == 'Enter')
        {
            if (_popup_data) _popup_data();
            _popup_data = null;            
        }
    };
}

function popup_clicked()
{
    const elem = I("popup-text");
    elem.classList.remove("fgl-show");
    window.onclick = null;
    document.onkeydown = null;
    if (_popup_data) _popup_data();
    _popup_data = null;
}

function send_ajax_request(url, callback = null, as_json = false)
{
    let req = new XMLHttpRequest();
    if (callback) req.onload = () => {callback(as_json ? JSON.parse(req.responseText) : req.responseText);};
    req.open("GET", "../action/" + url);
    req.send();
//    console.log('AJAX: ' + url);
}

function send_ajax_update_challenge(data, callback)
{
    let req = new XMLHttpRequest();
    req.onload = () => {callback(req.responseText);};
    req.open("POST", "../action/update_challenge.html");
    req.send(data);
}


function send_ajax_bg_add(data, callback)
{
    let req = new XMLHttpRequest();
    req.onload = () => {callback(req.responseText);};
    req.open("POST", "../action/bg_add.html");
    req.send(data);
}

// callback argument - new user name to put on door
function send_gift_load_msg(door_index, user_index, callback)
{
    send_ajax_request(`gift_load.html?door=${door_index}&user=${user_index}`, callback);
}

// callback argument - array of users (to put on doors)
function send_gift_unload_message(door_index, callback)
{
    send_ajax_request(`unload_gift.html?door=${door_index}`, callback, true);
}

function send_door_open_message(door_index)
{
    send_ajax_request(`open_door.html?door=${door_index}`);
}

// callback argument - array [HTML string with list of active Users, HTML string with list of done users]
function send_user_done(user_index, callback)
{
    send_ajax_request(`done_user.html?user=${user_index}`, callback, true);
}

function send_set_interround_time(time)
{
    send_ajax_request(`set_interround_time.html?value=${time}`);
}

function send_del_challenge(index)
{
    send_ajax_request(`del_challenge.html?index=${index}`);
}

// Callback called with text of challenge definition
function send_get_challenge(index, callback)
{
    send_ajax_request(`get_challenge.html?index=${index}`, callback);
}

// Callback called with JSON data with User options
function send_get_user_opts(index, callback)
{
    send_ajax_request(`get_user_opts.html?index=${index}`, callback, true);
}

// Callback called with new user name (to show in list nox)
function send_set_user_option(index, opt_name, opt_value, callback)
{
    send_ajax_request(`set_user_opt.html?index=${index}&name=${opt_name}&value=${encodeURIComponent(opt_value)}`);
}

function send_del_user(index)
{
    send_ajax_request(`del_user.html?index=${index}`);
}

function send_del_fg_user(user_index, fg_index)
{
    send_ajax_request(`del_user_fg.html?usr_index=${user_index}&fg_index=${fg_index}`);
}

function send_ping({cmd, cnt})
{
    send_ajax_request(`ping.html?id=${cmd}&cnt=${cnt}`);
}
