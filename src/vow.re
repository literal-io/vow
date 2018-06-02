module Vow = {
  type handled;
  type unhandled;
  type t('a, 'status) = {promise: Js.Promise.t('a)};
  let return = x => {promise: Js.Promise.resolve(x)};
  let fail = x => {promise: Js.Promise.reject(x)};
  let flatMap = (transform, vow) => {
    promise: Js.Promise.then_(x => transform(x).promise, vow.promise)
  };
  let flatMapUnhandled = (transform, vow) => {
    promise: Js.Promise.then_(x => transform(x).promise, vow.promise)
  };
  let map = (transform, vow) => flatMap(x => return(transform(x)), vow);
  let mapUnhandled = (transform, vow) =>
    flatMapUnhandled(x => return(transform(x)), vow);
  let sideEffect = (handler, vow) => {
    let _ =
      Js.Promise.then_(x => Js.Promise.resolve @@ handler(x), vow.promise);
    ();
  };
  let onError = (handler, vow) => {
    promise: Js.Promise.catch(err => handler(err).promise, vow.promise)
  };
  let wrap = promise => {promise: promise};
  let unsafeWrap = promise => {promise: promise};
  let unwrap = ({promise}) => promise;
  let unsafeUnwrap = ({promise}) => promise;
  let all2 = ((v1, v2)) =>
    v1
    |> flatMap(v1Result =>
         v2 |> flatMap(v2Result => return((v1Result, v2Result)))
       );
  let all3 = ((v1, v2, v3)) =>
    all2((v1, v2))
    |> flatMap(((v1Result, v2Result)) =>
         v3 |> flatMap(v3Result => return((v1Result, v2Result, v3Result)))
       );
  let all4 = ((v1, v2, v3, v4)) =>
    all3((v1, v2, v3))
    |> flatMap(((v1Result, v2Result, v3Result)) =>
         v4
         |> flatMap(v4Result =>
              return((v1Result, v2Result, v3Result, v4Result))
            )
       );
  let rec all = vows =>
    switch (vows) {
    | [head, ...tail] =>
      all(tail)
      |> flatMap(tailResult =>
           head |> flatMap(headResult => return([headResult, ...tailResult]))
         )
    | [] => return([])
    };

};

module type ResultType = {
  open Vow;
  type result('value, 'error);
  type vow('a, 'status) = t('a, 'status);
  type t('value, 'error, 'status) = vow(result('value, 'error), 'status);
  let return: 'value => t('value, 'error, handled);
  let fail: 'error => t('value, 'error, handled);
  let flatMap:
    ('a => t('b, 'error, 'status), t('a, 'error, handled)) =>
    t('b, 'error, 'status);
  let flatMapUnhandled:
    ('a => t('b, 'error, 'status), t('a, 'error, unhandled)) =>
    t('b, 'error, unhandled);
  let map: ('a => 'b, t('a, 'error, handled)) => t('b, 'error, 'status);
  let mapUnhandled:
    ('a => 'b, t('a, 'error, unhandled)) => t('b, 'error, unhandled);
  let mapError:
    ('a => t('value, 'b, handled), t('value, 'a, 'status)) =>
    t('value, 'b, 'status);
  let sideEffect:
    (
      [ | `Success('value) | `Fail('error)] => unit,
      t('value, 'error, handled)
    ) =>
    unit;
  let onError:
    (
      Js.Promise.error => t('error, 'value, 'status),
      t('error, 'value, unhandled)
    ) =>
    t('error, 'value, 'status);
  let wrap:
    (Js.Promise.t('value), Js.Promise.error => 'error) =>
    t('value, 'error, handled);
  let wrapOption:
    (unit => 'error, option('value)) => t('value, 'error, handled);
  let unwrap:
    (
      [ | `Success('value) | `Fail('error)] => vow('a, 'status),
      t('value, 'error, handled)
    ) =>
    vow('a, 'status);
  let unsafeUnwrap:
    ('error => exn, t('value, 'error, 'status)) => Js.Promise.t('value);

  let all2:
    ((t('v1, 'error, handled), t('v2, 'error, handled))) =>
    t(('v1, 'v2), 'error, handled);
  let all3:
    (
      (
        t('v1, 'error, handled),
        t('v2, 'error, handled),
        t('v3, 'error, handled),
      )
    ) =>
    t(('v1, 'v2, 'v3), 'error, handled);
  let all4:
    (
      (
        t('v1, 'error, handled),
        t('v2, 'error, handled),
        t('v3, 'error, handled),
        t('v4, 'error, handled),
      )
    ) =>
    t(('v1, 'v2, 'v3, 'v4), 'error, handled);
  let all:
    list(t('value, 'error, handled)) => t(list('value), 'error, handled);

  module Infix: {

    let (>>=):
      (t('a, 'error, handled), 'a => t('b, 'error, 'status)) =>
      t('b, 'error, 'status');
    let (>|=): (t('a, 'error, handled), 'a => 'b) => t('b, 'error, handled);
  };
};

module Result: ResultType = {
  type result('value, 'error) = [ | `Success('value) | `Fail('error)];
  type vow('a, 'status) = Vow.t('a, 'status);
  type t('value, 'error, 'status) = vow(result('value, 'error), 'status);
  let return = value => Vow.return(`Success(value));
  let fail = error => Vow.return(`Fail(error));
  let flatMap = (transform, vow) =>
    Vow.flatMap(
      x =>
        switch x {
        | `Success(x) => transform(x)
        | `Fail(x) => fail(x)
        },
      vow
    );
  let flatMapUnhandled = (transform, vow) =>
    Vow.flatMapUnhandled(
      x =>
        switch x {
        | `Success(x) => transform(x)
        | `Fail(x) => fail(x)
        },
      vow
    );
  let map = (transform, vow) => flatMap(x => return(transform(x)), vow);
  let mapUnhandled = (transform, vow) =>
    flatMapUnhandled(x => return(transform(x)), vow);
  let mapError = (transform, vow) =>
    Vow.flatMap(
      x =>
        switch x {
        | `Success(x) => return(x)
        | `Fail(x) => transform(x)
        },
      vow
    );
  let sideEffect = (handler, vow) => Vow.sideEffect(handler, vow);
  let onError = (handler, vow) => Vow.onError(handler, vow);
  let wrap = (promise, handler) =>
    Vow.wrap(promise)
    |> Vow.flatMapUnhandled(x => return(x))
    |> onError(err => fail(handler(err)));
  let wrapOption = (handler, option) =>
    switch option {
    | Some(value) => return(value)
    | None => fail(handler())
    };
  let unwrap = (transform, vow) => Vow.flatMap(transform, vow);
  let unsafeUnwrap = (onError, vow) =>
    Js.Promise.then_(
      x =>
        switch x {
        | `Success(x) => Js.Promise.resolve(x)
        | `Fail(x) => Js.Promise.reject(onError(x))
        },
      vow.Vow.promise
    );
  let all2 = ((v1, v2)) =>
    v1
    |> flatMap(v1Result =>
         v2 |> flatMap(v2Result => return((v1Result, v2Result)))
       );
  let all3 = ((v1, v2, v3)) =>
    all2((v1, v2))
    |> flatMap(((v1Result, v2Result)) =>
         v3 |> flatMap(v3Result => return((v1Result, v2Result, v3Result)))
       );
  let all4 = ((v1, v2, v3, v4)) =>
    all3((v1, v2, v3))
    |> flatMap(((v1Result, v2Result, v3Result)) =>
         v4
         |> flatMap(v4Result =>
              return((v1Result, v2Result, v3Result, v4Result))
            )
       );
  let rec all = vows =>
    switch (vows) {
    | [head, ...tail] =>
      all(tail)
      |> flatMap(tailResult =>
           head |> flatMap(headResult => return([headResult, ...tailResult]))
         )
    | [] => return([])
    };
  module Infix = {
    let (>>=) = (v, t) => flatMap(t, v);
    let (>|=) = (v, t) => map(t, v);
  };
};

include Vow;