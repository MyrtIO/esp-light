import type {
  Configuration,
  LightConfiguration,
  LightTestRequest,
  OtaUploadResult,
  SystemInformation,
} from "../models";

export interface ApiService {
  getConfiguration(): Promise<Configuration>;
  saveConfiguration(configuration: Configuration): Promise<void>;
  setLightConfiguration(light: LightConfiguration): Promise<void>;
  testColor(request: LightTestRequest): Promise<void>;
  getSystemInformation(): Promise<SystemInformation>;
  uploadFirmware(
    file: File,
    onProgress?: (progress: number) => void
  ): Promise<OtaUploadResult>;
}
