"use strict";

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
    return '<table class="door-table">' + result + '</table>';
}

function loaded_gifts_to_disabled(second_names)
{
    let result = 0;
    let scale = 1;
    for(let x of second_names)
    {
        if (x) result |= scale;
        scale <<= 1;
    }
    return result;
}

// let time_to_doors_enable = ${TimeToDoorsEnable};
let _time_to_doors_enable = 10;

function _msg_time_to_doors()
{
    document.getElementById("downcounter").innerHTML = _time_to_doors_enable ? "Вы сможете открыть дверцу через " + _time_to_doors_enable + " секунд" : "";
}

function activate_doors(second_names, disabled_scale, usr_callback, reset_timeout = false)
{
    if (reset_timeout) _time_to_doors_enable = 10;
    _msg_time_to_doors();
    let doors = document.getElementById("doors");
    if (_time_to_doors_enable)
    {
        doors.innerHTML = door_table(second_names, 255, "");
        let cb = function()
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
    }
}

function send_ajax_request(url, callback = null, as_json = false)
{
    let req = new XMLHttpRequest();
    if (callback) req.onload = function() {callback(as_json ? JSON.parse(this.responseText) : this.responseText);};
    req.open("GET", "../action/" + url);
    req.send();
}

// callback argument - new user name to put on door
function send_gift_load_msg(door_index, user_index, callback)
{
    send_ajax_request('gift_load.html?door=' + door_index +'&user=' + user_index, callback);
}

// callback argument - array of users (to put on doors)
function send_gift_unload_message(door_index, callback)
{
    send_ajax_request('unload_gift.html?door=' + door_index, callback, true);
}

function send_door_open_message(door_index)
{
    send_ajax_request('open-door.html?door=' + door_index);
}
