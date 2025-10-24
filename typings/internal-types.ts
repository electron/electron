/**
 * Internal type definitions for Electron
 * This file contains proper TypeScript interfaces to replace 'any' types
 */

/// <reference path="../electron.d.ts" />

/**
 * Touch bar interaction details
 */
export interface TouchBarInteractionDetails {
  /** The button that was pressed */
  button?: string;
  /** The key that was pressed */
  key?: string;
  /** Keyboard modifiers that were active */
  modifiers?: string[];
  /** Timestamp of the interaction */
  timestamp?: number;
}

/**
 * Global values that can be set in context bridge
 */
export type GlobalValue = string | number | boolean | object | Function | null | undefined;

/**
 * IPC message types
 */
export type IPCMessage = string | number | boolean | object | null | undefined;

/**
 * Transferable objects for IPC
 */
export type TransferableObject = ArrayBuffer | MessagePort | ImageBitmap | OffscreenCanvas;

/**
 * Internal print options
 */
export interface InternalPrintOptions {
  silent?: boolean;
  printBackground?: boolean;
  color?: boolean;
  margin?: {
    marginType?: 'default' | 'none' | 'printableArea' | 'custom';
    top?: number;
    right?: number;
    bottom?: number;
    left?: number;
  };
  landscape?: boolean;
  scaleFactor?: number;
  pagesPerSheet?: number;
  collate?: boolean;
  copies?: number;
  header?: string;
  footer?: string;
  pageSize?: string | { width: number; height: number };
}

/**
 * Window open handler result
 */
export interface WindowOpenHandlerResult {
  browserWindowConstructorOptions: Electron.BrowserWindowConstructorOptions | null;
  outlivesOpener: boolean;
  createWindow?: (options: Electron.BrowserWindowConstructorOptions) => Electron.WebContents;
}

/**
 * Window open event
 */
export interface WindowOpenEvent {
  preventDefault(): void;
  defaultPrevented: boolean;
  url: string;
  frameName: string;
  disposition: 'default' | 'foreground-tab' | 'background-tab' | 'new-window' | 'save-to-disk' | 'other';
  features: string;
  referrer: { url: string; policy: string };
  postBody?: { data: Buffer; contentType: string } | null;
}

/**
 * Menu delegate interface
 */
export interface MenuDelegate {
  getAcceleratorForCommandId?(commandId: string): string | undefined;
  isCommandIdChecked?(commandId: string): boolean;
  isCommandIdEnabled?(commandId: string): boolean;
  isCommandIdVisible?(commandId: string): boolean;
  shouldCommandIdWorkWhenHidden?(commandId: string): boolean;
  executeCommand?(commandId: string, event?: KeyboardEvent): void;
}

/**
 * Protocol handler interface
 */
export interface ProtocolHandler {
  (request: {
    url: string;
    method: string;
    referrer: string;
    headers: Record<string, string>;
    uploadData?: Array<{ bytes: Buffer; file: string }>;
  }, callback: (response: {
    statusCode?: number;
    statusMessage?: string;
    headers?: Record<string, string>;
    data?: Buffer | string;
  }) => void): void;
}

/**
 * Utility process message types
 */
export type UtilityProcessMessage = 
  | { type: 'init'; data: object }
  | { type: 'task'; data: object }
  | { type: 'result'; data: object }
  | { type: 'error'; error: string };

/**
 * IPC event listener arguments
 */
export type IPCEventArgs = IPCMessage[];

/**
 * IPC ports array
 */
export type IPCPorts = TransferableObject[];

/**
 * Module loader function
 */
export type ModuleLoader = () => unknown;

/**
 * Send message callback result
 */
export type SendMessageCallbackResult = unknown;
