inputs: { options, config, lib, pkgs, ... }:

with lib; let
  inherit (pkgs.stdenv.hostPlatform) system;
  cfg = config.services.k95aux;
  isEmpty = attr: length (attrNames attr) == 0;
in {
  options.services.k95aux = with types; {
    enable = mkEnableOption "Auxillary Corsair K95 keys";
    package = mkOption {
      type = package;
      default = inputs.self.packages.${system}.k95aux;
    };
    mapping = mkOption {
      type = attrsOf types.ints.u8;
      default = {};
    };
  };

  config = let
    mapping = strings.concatMapAttrsStringSep "\n" (key: code: "${key} ${toString code}") cfg.mapping;
  in mkIf cfg.enable {
    systemd.services.k95aux = {
      description = "Auxillary Corsair K95 keys service";
      wantedBy = [ "multi-user.target" ];

      serviceConfig = {
        ExecStart = "${cfg.package}/bin/k95aux";
        Restart = "on-failure";
      };

      restartTriggers = [
        mapping
      ];
    };

    environment.etc = mkIf (!isEmpty cfg.mapping) {
      "k95aux/mapping".text = mapping;
    };
  };
}
