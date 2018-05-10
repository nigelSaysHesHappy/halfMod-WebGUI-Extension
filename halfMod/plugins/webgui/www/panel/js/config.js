$(function () {
    //"use strict";

    // for better performance - to avoid searching in DOM
    var content = $('#content');
    var status = $('#status');

    // if user is running mozilla then use it's built-in WebSocket
    window.WebSocket = window.WebSocket || window.MozWebSocket;

    // if browser doesn't support WebSocket, just show some notification and exit
    if (!window.WebSocket) {
        content.html($('<p>', { text: 'Sorry, but your browser doesn\'t '
                                    + 'support WebSockets.'} ));
        return;
    }

    // open connection
    window.wsButtonConnection = new WebSocket('ws://' + window.location.hostname + ':9001/panel/ws/config');

    window.wsButtonConnection.onerror = function (error) {
        // just in there were some problems with conenction...
        content.html($('<p>', { text: 'Sorry, but there\'s some problem with your '
                                    + 'connection or the server is down.' } ));
    };

    var complete = false;
    // most important part - incoming messages
    window.wsButtonConnection.onmessage = function (message) {
        if (complete) status.text(message.data);
        else {
            if (message.data == "complete") complete = true;
            else addButton(message.data);
        }
    };

    function addButton(message) {
        content.append(message);
    }
});
