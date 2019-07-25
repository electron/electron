export const ifit = (condition: boolean) => (condition ? it : it.skip)
export const ifdescribe = (condition: boolean) => (condition ? describe : describe.skip)