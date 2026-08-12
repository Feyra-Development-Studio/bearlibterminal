// Placeholder for Pascal bindings and extensions for dungeon generator integration
// This file will be expanded with classes and the integration code.

unit BLT_Dungeon;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils;

type
  TBLTIntegration = class
  public
    constructor Create;
    destructor Destroy; override;
    procedure Initialize; virtual;
  end;

implementation

constructor TBLTIntegration.Create;
begin
  inherited Create;
end;

destructor TBLTIntegration.Destroy;
begin
  inherited Destroy;
end;

procedure TBLTIntegration.Initialize;
begin
  // initialization placeholder
end;

end.
