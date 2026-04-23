import type {
  Configuration,
  LightConfiguration,
  LightTestRequest,
  SystemInformation,
} from "../models";
import type { ApiService } from "./interface";

export class MockApiService implements ApiService {
  async getConfiguration(): Promise<Configuration> {
    console.log(`[mock] getting configuration`);
    await simulateNetworkDelay();
    return {
      wifi: {
        ssid: "MySSID",
        password: "My Password",
      },
      mqtt: {
        host: "mqtt-balancer.lan",
        port: 1883,
        username: "myrtio",
        password: "myrtio",
      },
      light: {
        brightness_min: 0,
        brightness_max: 255,
        led_count: 26,
        skip_leds: 0,
        color_correction: 0xf7e4ff,
        color_order: "grb",
      },
    };
  }

  async saveConfiguration(configuration: Configuration): Promise<void> {
    console.log(`[mock] saving configuration`, {
      configuration,
    });
    await simulateNetworkDelay();
    return;
  }

  async setLightConfiguration(light: LightConfiguration): Promise<void> {
    console.log(`[mock] setting light configuration`, { light });
    await simulateNetworkDelay();
    return;
  }

  async testColor(request: LightTestRequest): Promise<void> {
    console.log(`[mock] testing color`, { request });
    await simulateNetworkDelay();
    return;
  }

  async getSystemInformation(): Promise<SystemInformation> {
    console.log(`[mock] getting system information`);
    await simulateNetworkDelay();
    return {
      build_version: "0d35914-2025-12-23T11:16:09+0000",
      network_mode: "AP + STA",
      sta_ip: "192.168.1.34",
      ap_ip: "192.168.4.1",
      mac_address: [160, 183, 101, 22, 48, 84],
    };
  }
}

function simulateNetworkDelay(
  min: number = 500,
  delta: number = 1000
): Promise<void> {
  let delay = Math.random() * delta + min;
  return sleep(delay);
}

const sleep = (ms: number): Promise<void> =>
  new Promise((resolve) => setTimeout(resolve, ms));
