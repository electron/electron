export function warnOnce (oldName: string, newName?: string) {
  return warnOnceMessage(newName
    ? `'${oldName}' is deprecated and will be removed. Please use '${newName}' instead.`
    : `'${oldName}' is deprecated and will be removed.`);
}

export function warnOnceMessage (msg: string) {
  let warned = false;
  return () => {
    if (!warned) {
      warned = true;
      log(msg);
    }
  };
}

export function warn (oldName: string, newName: string): void {
  log(`'${oldName}' is deprecated. Use '${newName}' instead.`);
}

export function log (message: string): void {
  if (process.emitWarning) {
    process.emitWarning(message, 'DeprecationWarning');
  } else {
    // Fallback if process.emitWarning is not available,
    // such as a renderer process without Node integration
    console.warn(`(electron) ${message}`);
  }
}

// remove a function with no replacement
export function removeFunction<T extends Function> (fn: T, removedName: string): T {
  if (!fn) { throw new Error(`'${removedName} function' is invalid or does not exist.`); }

  // wrap the deprecated function to warn user
  const warn = warnOnce(`${fn.name} function`);
  return function (this: any) {
    warn();
    fn.apply(this, arguments);
  } as unknown as typeof fn;
}

// change the name of a function
export function renameFunction<T extends Function> (fn: T, newName: string): T {
  const warn = warnOnce(`${fn.name} function`, `${newName} function`);
  return function (this: any) {
    warn();
    return fn.apply(this, arguments);
  } as unknown as typeof fn;
}

// change the name of an event
export function event (emitter: NodeJS.EventEmitter, oldName: string, newName: string, transformer: (...args: any[]) => any[] | undefined = (...args) => args) {
  const warn = newName.startsWith('-') /* internal event */
    ? warnOnce(`${oldName} event`)
    : warnOnce(`${oldName} event`, `${newName} event`);
  return emitter.on(newName, function (this: NodeJS.EventEmitter, ...args) {
    if (this.listenerCount(oldName) !== 0) {
      warn();
      const transformedArgs = transformer(...args);
      if (transformedArgs) {
        this.emit(oldName, ...transformedArgs);
      }
    }
  });
}

// remove a property with no replacement
export function removeProperty<T extends Object, K extends (keyof T & string)>(object: T, removedName: K, onlyForValues?: any[]): T {
  // if the property's already been removed, warn about it
  const info = Object.getOwnPropertyDescriptor(object, removedName);
  if (!info) {
    log(`Unable to remove property '${removedName}' from an object that lacks it.`);
    return object;
  }
  if (!info.get || !info.set) {
    log(`Unable to remove property '${removedName}' from an object does not have a getter / setter`);
    return object;
  }

  // wrap the deprecated property in an accessor to warn
  const warn = warnOnce(removedName);
  return Object.defineProperty(object, removedName, {
    configurable: true,
    get: () => {
      warn();
      return info.get!.call(object);
    },
    set: newVal => {
      if (!onlyForValues || onlyForValues.includes(newVal)) {
        warn();
      }
      return info.set!.call(object, newVal);
    }
  });
}

// change the name of a property
export function renameProperty<T extends Object, K extends (keyof T & string)>(object: T, oldName: string, newName: K): T {
  const warn = warnOnce(oldName, newName);

  // if the new property isn't there yet,
  // inject it and warn about it
  if ((oldName in object) && !(newName in object)) {
    warn();
    object[newName] = (object as any)[oldName];
  }

  // wrap the deprecated property in an accessor to warn
  // and redirect to the new property
  return Object.defineProperty(object, oldName, {
    get: () => {
      warn();
      return object[newName];
    },
    set: value => {
      warn();
      object[newName] = value;
    }
  });
}

export function moveAPI<T extends Function> (fn: T, oldUsage: string, newUsage: string): T {
  const warn = warnOnce(oldUsage, newUsage);
  return function (this: any) {
    warn();
    return fn.apply(this, arguments);
  } as unknown as typeof fn;
}
