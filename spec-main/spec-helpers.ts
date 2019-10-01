export const ifit = (condition: boolean) => (condition ? it : it.skip)
export const ifdescribe = (condition: boolean) => (condition ? describe : describe.skip)

export const delay = (time: number) => new Promise(r => setTimeout(r, time))
