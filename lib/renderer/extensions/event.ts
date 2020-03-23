export class Event {
  private listeners: Function[] = []

  addListener (callback: Function) {
    this.listeners.push(callback);
  }

  removeListener (callback: Function) {
    const index = this.listeners.indexOf(callback);
    if (index !== -1) {
      this.listeners.splice(index, 1);
    }
  }

  emit (...args: any[]) {
    for (const listener of this.listeners) {
      listener(...args);
    }
  }
}
