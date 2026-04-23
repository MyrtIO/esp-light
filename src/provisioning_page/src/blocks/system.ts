import type { MacAddress, SystemInformation } from "../models";

export class SystemBlock {
  private buildVersion: HTMLDivElement;
  private networkMode: HTMLDivElement;
  private staIp: HTMLDivElement;
  private apIp: HTMLDivElement;
  private macAddress: HTMLDivElement;
  private block: HTMLElement;

  constructor(section: HTMLElement) {
    const $ = <T extends HTMLElement = HTMLDivElement>(selector: string): T => {
      const element = section.querySelector<T>(selector);
      if (!element) {
        throw new Error(`Element with selector ${selector} not found`);
      }
      return element;
    };

    this.block = section;
    this.buildVersion = $("#build-version");
    this.networkMode = $("#network-mode");
    this.staIp = $("#sta-ip");
    this.apIp = $("#ap-ip");
    this.macAddress = $("#mac-address");
  }

  public setValues(system: SystemInformation) {
    this.buildVersion.textContent = system.build_version;
    this.networkMode.textContent = system.network_mode;
    this.staIp.textContent = system.sta_ip || "нет подключения";
    this.apIp.textContent = system.ap_ip || "выключен";
    this.macAddress.textContent = formatMacAddress(system.mac_address);
  }

  public unlock() {
    this.block.classList.remove("_disabled");
  }

  public lock() {
    this.block.classList.add("_disabled");
  }
}

function formatMacAddress(macAddress: MacAddress): string {
  return macAddress.map((byte) => byte.toString(16).padStart(2, "0")).join(":");
}
