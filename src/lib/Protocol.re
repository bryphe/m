[@deriving yojson({strict: false})]
type requestMessage('t) = {
  id: int,
  method: string,
  params: 't,
};

[@deriving yojson({strict: false})]
type responseMessage('t) = {
  id: int,
  result: 't,
};

[@deriving yojson({strict: false})]
type initializeRequestParams = {
  rootUri: string,
};

[@deriving yojson({strict: false})]
type debugEchoParams = {
   message: string 
};

[@deriving yojson({strict: false})]
type initializeRequest = requestMessage(initializeRequestParams);

[@deriving yojson({strict: false})]
type debugEchoRequest = requestMessage(debugEchoParams);

[@deriving yojson({strict: false})]
type serverCapabilities = {textDocumentSync: int};

[@deriving yojson({strict: false})]
type initializeResult = {capabilities: serverCapabilities};

[@deriving yojson({strict: false})]
type initializeResponse = responseMessage(initializeResult);

[@deriving yojson({strict: false})]
type debugEchoResult = responseMessage(debugEchoParams);

type request =
  | Initialize(initializeRequestParams)
  | DebugEcho(debugEchoParams)
  | UnknownRequest;

type notification =
  | Exit
  | UnknownNotification;

type response =
  | InitializeResult(initializeResult)
  | DebugEchoResult(debugEchoResult)
  | UnknownResponse;

type message =
  | Request(int, request)
  | Notification(notification)
  | Response(response);

let hasField = (f: string, msg: Yojson.Safe.json) => {
  let v = Yojson.Safe.Util.member(f, msg);

  switch (v) {
  | `Null => false
  | _ => true
  };
};

let hasMethod = hasField("method");
let hasId = hasField("id");
let hasResult = hasField("result");

let isNotification = (msg: Yojson.Safe.json) =>
  hasMethod(msg) && !hasId(msg);

let isRequest = (msg: Yojson.Safe.json) => hasMethod(msg) && hasId(msg);

let isResponse = (msg: Yojson.Safe.json) => hasResult(msg) && hasId(msg);

exception ParseException(string);

let getOrThrow = (r: Result.result('t, string)) =>
  switch (r) {
  | Ok(v) => v
  | Error(e) => raise(ParseException("getOrThrow: Error parsing: " ++ e))
  };

let parseNotification = (msg:Yojson.Safe.json) => {
  let method =
    msg |> Yojson.Safe.Util.member("method") |> Yojson.Safe.Util.to_string;

  switch(method) {
  | "exit" => Exit
  | _ => UnknownNotification
  }
    
}

let parseRequest = (msg: Yojson.Safe.json) => {
  let method =
    msg |> Yojson.Safe.Util.member("method") |> Yojson.Safe.Util.to_string;

  prerr_endline ("GOT METHOD: " ++ method);

  switch (method) {
  | "initialize" =>
    let v = initializeRequest_of_yojson(msg) |> getOrThrow;
    Initialize(v.params);
  | "debug/echo" =>
    let v = debugEchoRequest_of_yojson(msg) |> getOrThrow;
    DebugEcho(v.params);
  | _ => UnknownRequest
  };
};

let parse: string => message =
  msg => {
    let p = Yojson.Safe.from_string(msg);

    switch (isNotification(p), isRequest(p), isResponse(p)) {
    | (true, _, _) => 
        let n = parseNotification(p);
        Notification(n);
    | (_, true, _) => 
        let id = p |> Yojson.Safe.Util.member("id") |> Yojson.Safe.Util.to_int;
        prerr_endline ("GOT ID: " ++ string_of_int(id));
        Request(id, parseRequest(p))
    | _ => Response(UnknownResponse)
    };
  };
