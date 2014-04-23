function ajax(url, method, data, callback) {
  var req = new XMLHttpRequest();
  req.open(method, url, true);
  req.onload = function(e) {
    if (req.readyState === 4 && req.status === 200) {
      if (req.status === 200) {
        callback(JSON.parse(req.responseText));
      } else {
        console.log("HTTP Error: " + JSON.stringify(req));
      }
    }
  };
  req.send(data);
}

function getReq(url, callback) {
  return ajax(url, 'GET', null, callback);
}

var bytearray;
var last_sent_time = 0;
var config;
var roomId;

function gen_message(question_number, answer){
  return { "question_number": question_number, "answer": answer };
}

function send_msg_success( e ){
  console.log("SUCCESS - " + e);
}

function send_msg_fail( e ){
  console.log(e.error.message);
}

function parseFireBaseAnswerData(data) {
  var table = [];
  var bytearray = [];

  for (var key in data) {
    var response = data[key];

    if (table[response.question_number] === undefined) {
      table[response.question_number] = [];
    }
    if (response.answer !== 0) {
      table[response.question_number].push(response.answer);
    }
  }

  for (var key in data) {
    var response = data[key];
    if (table[response.question_number] !== undefined &&
        table[response.question_number].length === 0) {
      delete(table[response.question_number]);
    }
  }

  for (var i = 0; i < table.length; i++) {
    var list = table[i];
    if (list === undefined) {
      continue;
    }

    console.log(list);

    for (var j = 0; j < list.length; j++) {
      bytearray.push(i);
      bytearray.push(list[j]);
    }
  }

  Pebble.sendAppMessage({"2":bytearray});
}

var getAndSetRoomId = function() {
  console.log("Ready begin");
  config = window.localStorage.getItem('config');
  if(config) {
    var roomId = config["room-id"];
  } else {
    Pebble.showSimpleNotificationOnPebble("Woah there!",
                                          "Set the room inside of Clock on your phone.");
  }
}

Pebble.addEventListener("ready", function(e) {

  Firebase.INTERNAL.forceWebSockets();

  // localStorage.removeItem('config');

  getAndSetRoomId();

  console.log("Ready - " + roomId);

  var fb = new Firebase('https://kirby.firebaseio.com/rooms/' + roomId);

  var first = true; // so we don't get one unless it's new

  fb.endAt().limit(1).on('child_added', function(snapshot) {
    if(!first) {
      var data = snapshot.val();
      var current_time = Date.now();
      var time_diff = current_time - last_sent_time;
      if ( time_diff > 100 ){
        var msg = gen_message(data.question_number, data.answer);
        Pebble.sendAppMessage( msg, send_msg_success, send_msg_fail );
      }
    } else {
      first = false;
    }
  });
  getReq('https://kirby.firebaseio.com/rooms/' + roomId + '/.json', parseFireBaseAnswerData);

  fb.on('value', function (snapshot) {
    var data = snapshot.val();
    parseFireBaseAnswerData(data);
  });
  console.log("Ready end");
});

Pebble.addEventListener("appmessage", function(e) {
  var roomId = config['room-id'];
  console.log("AppMessage - " + roomId);
  var fb = new Firebase('https://kirby.firebaseio.com/rooms/' + roomId );
  var msg = e.payload;

  var new_msg = {'question_number': 0, 'answer': 0};

  if ( typeof msg.question_number !== undefined ){
    new_msg.question_number = msg.question_number;
  }
  if ( msg.answer && typeof msg.answer !== undefined ){
    new_msg.answer = msg.answer;
  }
  last_sent_time = Date.now();
  fb.push( new_msg );
});

Pebble.addEventListener("showConfiguration", function (e) {
  var url = "https://www.cs.purdue.edu/homes/sopell/clock-config.html";
  Pebble.openURL(url);
});

Pebble.addEventListener("webviewclosed", function (e) {
  if (!e.response){
    return;
  }
  var json = decodeURIComponent( e.response );
  config = JSON.parse( json );

  console.log("room id is " + config["room-id"] );
  console.log("vibration status is " + config["vib-toggle"] );

  window.localStorage.setItem( 'config', config );
  getAndSetRoomId();
});
