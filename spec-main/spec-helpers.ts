export const ifit = (condition: boolean) => (condition ? it : it.skip);
export const ifdescribe = (condition: boolean) => (condition ? describe : describe.skip);

export const delay = (time: number = 0) => new Promise(resolve => setTimeout(resolve, time));
