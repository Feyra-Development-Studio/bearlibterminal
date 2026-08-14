program scene;
{ Проба для задачи #9: паскалевская программа без единой строчки защиты.

  Здесь нарочно НЕ вызывается SetExceptionMask. До правки эта программа
  падала с EInvalidOp на первом же кадре: Паскаль, в отличие от C,
  размаскировывает исключения с плавающей точкой, а OpenGL их вызывает при
  обычной работе. Смысл проверки в том, чтобы поймать возврат этого поведения
  — например, если маску из initialization уберут как «лишнюю». }

{$MODE OBJFPC}{$H+}

uses
  SysUtils, BearLibTerminal;

var
  LogFile: AnsiString;
begin
  if ParamCount < 1 then
  begin
    WriteLn('нужен путь к файлу журнала');
    Halt(2);
  end;
  LogFile := ParamStr(1);

  if not terminal_open then
  begin
    WriteLn('terminal_open не отработал');
    Halt(1);
  end;

  terminal_set('log: file=' + LogFile + ', level=info');
  terminal_set('window: size=20x5, title=''pascal-check''; font: default');

  terminal_clear;
  terminal_color(color_from_name('white'));
  terminal_print(1, 1, 'жив');
  terminal_refresh;

  { Запись в журнал из Паскаля — заодно проверяется привязка из задачи #3. }
  terminal_log(TK_LOG_INFO, 'паскалевская проба дошла до кадра');

  terminal_delay(300);
  terminal_close;

  WriteLn('дожил до конца');
end.
